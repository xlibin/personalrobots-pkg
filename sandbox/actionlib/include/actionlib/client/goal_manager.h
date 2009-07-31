/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#ifndef ACTIONLIB_GOAL_MANAGER_H_
#define ACTIONLIB_GOAL_MANAGER_H_

#include <boost/thread/recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


#include "actionlib/action_definition.h"

#include "actionlib/managed_list.h"
#include "actionlib/enclosure_deleter.h"
#include "actionlib/goal_id_generator.h"

#include "actionlib/client/comm_state.h"
#include "actionlib/client/terminal_state.h"

// msgs
#include "actionlib/GoalID.h"
#include "actionlib/GoalStatusArray.h"
#include "actionlib/RequestType.h"

namespace actionlib
{

//! \todo figure out why I get compile errors trying to use boost::mutex::scoped_lock()
class ScopedLock
{
public:
  ScopedLock(boost::recursive_mutex& mutex) : mutex_(mutex)  {  mutex_.lock(); }
  ~ScopedLock()  { mutex_.unlock(); }
private:
  boost::recursive_mutex& mutex_;
};

template <class ActionSpec>
class GoalHandle;

template <class ActionSpec>
class CommStateMachine;

template <class ActionSpec>
class GoalManager
{
public:
  ACTION_DEFINITION(ActionSpec);
  typedef GoalManager<ActionSpec> GoalManagerT;
  typedef boost::function<void (GoalHandle<ActionSpec>) > TransitionCallback;
  typedef boost::function<void (GoalHandle<ActionSpec>, const FeedbackConstPtr&) > FeedbackCallback;
  typedef boost::function<void (const ActionGoalConstPtr)> SendGoalFunc;
  typedef boost::function<void (const GoalID&)> CancelFunc;

  GoalManager() { }

  void registerSendGoalFunc(SendGoalFunc send_goal_func);
  void registerCancelFunc(CancelFunc cancel_func);

  GoalHandle<ActionSpec> initGoal( const Goal& goal,
                                   TransitionCallback transition_cb = TransitionCallback(),
                                   FeedbackCallback feedback_cb = FeedbackCallback() );

  void updateStatuses(const GoalStatusArrayConstPtr& status_array);
  void updateFeedbacks(const ActionFeedbackConstPtr& action_feedback);
  void updateResults(const ActionResultConstPtr& action_result);

  friend class GoalHandle<ActionSpec>;

  // should be private
  typedef ManagedList< boost::shared_ptr<CommStateMachine<ActionSpec> > > ManagedListT;
  ManagedListT list_;
private:
  SendGoalFunc send_goal_func_ ;
  CancelFunc cancel_func_ ;

  boost::recursive_mutex list_mutex_;

  GoalIDGenerator id_generator_;

  void listElemDeleter(typename ManagedListT::iterator it);
};

/**
 * \brief Client side handle to monitor goal progress
 *
 * A GoalHandle is a reference counted object that is used to manipulate and monitor the progress
 * of an already dispatched goal. Once all the goal handles go out of scope (or are reset), an
 * ActionClient stops maintaining state for that goal.
 */
template <class ActionSpec>
class GoalHandle
{
private:
  ACTION_DEFINITION(ActionSpec);

public:
  /**
   * \brief Create an empty goal handle
   *
   * Constructs a goal handle that doesn't track any goal. Calling any method on an empty goal
   * handle other than operator= will trigger an assertion.
   */
  GoalHandle();

  /**
   * \brief Stops goal handle from tracking a goal
   *
   * Useful if you want to stop tracking the progress of a goal, but it is inconvenient to force
   * the goal handle to go out of scope. Has pretty much the same semantics as boost::shared_ptr::reset()
   */
  void reset();

  /**
   * \brief Checks if this goal handle is tracking a goal
   * \return True if this goal handle is indeed tracking a goal
   */
  inline bool isActive() const;

  /**
   * \brief Get the state of this goal's communication state machine from interaction with the server
   *
   * Possible States are: WAITING_FOR_GOAL_ACK, PENDING, ACTIVE, WAITING_FOR_RESULT,
   *                      WAITING_FOR_CANCEL_ACK, RECALLING, PREEMPTING, DONE
   * \return The current goal's communication state with the server
   */
  CommState getCommState();

  /**
   * \brief Get the terminal state information for this goal
   *
   * Possible States Are: RECALLED, REJECTED, PREEMPTED, ABORTED, SUCCEEDED, LOST
   * This call only makes sense if CommState==DONE. This will send ROS_WARNs if we're not in DONE
   * \return The terminal state
   */
  TerminalState getTerminalState();

  /**
   * \brief Get result associated with this goal
   *
   * \return NULL if no reseult received.  Otherwise returns shared_ptr to result.
   */
  ResultConstPtr getResult();

  /**
   * \brief Resends this goal [with the same GoalID] to the ActionServer
   *
   * Useful if the user thinks that the goal may have gotten lost in transit
   */
  void resend();

  /**
   * \brief Sends a cancel message for this specific goal to the ActionServer
   *
   * Also transitions the Communication State Machine to WAITING_FOR_CANCEL_ACK
   */
  void cancel();

  friend class GoalManager<ActionSpec>;
private:
  typedef GoalManager<ActionSpec> GoalManagerT;
  typedef ManagedList< boost::shared_ptr<CommStateMachine<ActionSpec> > > ManagedListT;

  GoalHandle(GoalManagerT* gm, typename ManagedListT::Handle handle);

  GoalManagerT* gm_;
  bool active_;
  //typename ManagedListT::iterator it_;
  typename ManagedListT::Handle list_handle_;
};

template <class ActionSpec>
class CommStateMachine
{
  private:
    //generates typedefs that we'll use to make our lives easier
    ACTION_DEFINITION(ActionSpec);

  public:
    typedef boost::function<void (const GoalHandle<ActionSpec>&) > TransitionCallback;
    typedef boost::function<void (const GoalHandle<ActionSpec>&, const FeedbackConstPtr&) > FeedbackCallback;
    typedef GoalHandle<ActionSpec> GoalHandleT;

    CommStateMachine(const ActionGoalConstPtr& action_goal,
                     TransitionCallback transition_callback,
                     FeedbackCallback feedback_callback);

    ActionGoalConstPtr getActionGoal() const;
    CommState getCommState() const;
    GoalStatus getGoalStatus() const;
    ResultConstPtr getResult() const;

    // Transitions caused by messages
    void updateStatus(GoalHandleT& gh, const GoalStatusArrayConstPtr& status_array);
    void updateFeedback(GoalHandleT& gh, const ActionFeedbackConstPtr& feedback);
    void updateResult(GoalHandleT& gh, const ActionResultConstPtr& result);

    // Forced transitions
    void transitionToState(GoalHandleT& gh, const CommState::StateEnum& next_state);
    void transitionToState(GoalHandleT& gh, const CommState& next_state);
    void processLost(GoalHandleT& gh);
  private:
    CommStateMachine();

    // State
    CommState state_;
    ActionGoalConstPtr action_goal_;
    GoalStatus latest_goal_status_;
    ActionResultConstPtr latest_result_;

    // Callbacks
    TransitionCallback transition_cb_;
    FeedbackCallback   feedback_cb_;

    // **** Implementation ****
    //! Change the state, as well as print out ROS_DEBUG info
    void setCommState(const CommState& state);
    void setCommState(const CommState::StateEnum& state);
    const GoalStatus* findGoalStatus(const std::vector<GoalStatus>& status_vec) const;
};

}

#include "actionlib/client/goal_manager.cpp"
#include "actionlib/client/goal_handle.cpp"
#include "actionlib/client/comm_state_machine.cpp"

#endif // ACTIONLIB_GOAL_MANAGER_H_
