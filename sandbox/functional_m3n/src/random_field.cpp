/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Daniel Munoz
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

#include <functional_m3n/random_field.h>

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::RandomField(unsigned int nbr_clique_sets)
{
  clique_sets_.resize(nbr_clique_sets);
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::~RandomField()
{
  clear();
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
void RandomField::clear()
{
  // Free nodes (using random field id)
  for (map<unsigned int, Node*>::iterator iter_rf_nodes = rf_nodes_.begin() ; iter_rf_nodes
      != rf_nodes_.end() ; iter_rf_nodes++)
  {
    delete iter_rf_nodes->second;
  }

  // Free cliques in each clique set
  for (unsigned int i = 0 ; i < clique_sets_.size() ; i++)
  {
    for (map<unsigned int, Clique*>::iterator iter_cliques = clique_sets_[i].begin() ; iter_cliques
        != clique_sets_[i].end() ; iter_cliques++)
    {
      delete iter_cliques->second;
    }
  }

  // Empty data structures
  rf_nodes_.clear();
  clique_sets_.clear();
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
int RandomField::updateLabelings(const map<unsigned int, unsigned int>& new_labeling)
{
  map<unsigned int, unsigned int>::const_iterator iter_new_labeling;

#if ROS_DEBUG
  // Ensure the number of nodes equals the new labeling mapping
  if (rf_nodes_.size() != new_labeling.size())
  {
    ROS_ERROR("Inconsistent number of nodes (%u) with mapping size (%u)", rf_nodes_.size(), new_labeling.size());
    return -1;
  }

  // Ensure the keys (node id) in new_labeling exist in this RandomField
  for (iter_new_labeling = new_labeling.begin(); iter_new_labeling != new_labeling.end(); iter_new_labeling++)
  {
    if (rf_nodes_.count(iter_new_labeling->first) == 0)
    {
      ROS_ERROR("Unknown node id from the map: %u", iter_new_labeling->first);
      return -1;
    }
  }
#endif

  // -------------------------------------------------------
  // Update node labels
  for (iter_new_labeling = new_labeling.begin(); iter_new_labeling != new_labeling.end() ; iter_new_labeling++)
  {
    (rf_nodes_[iter_new_labeling->first])->setLabel(iter_new_labeling->second);
  }

  // -------------------------------------------------------
  // Update label information in each clique in the clique sets
  map<unsigned int, Clique*>::iterator iter_cliques;
  Clique* curr_clique = NULL;
  for (unsigned int i = 0 ; i < clique_sets_.size() ; i++)
  {
    map<unsigned int, Clique*>& curr_cs = clique_sets_[i];
    for (iter_cliques = curr_cs.begin(); iter_cliques != curr_cs.end() ; iter_cliques++)
    {
      curr_clique = iter_cliques->second;
      curr_clique->updateLabels(new_labeling);
    }
  }

  return 0;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
const RandomField::Node* RandomField::createNode(float* feature_vals,
                                                 unsigned int nbr_feature_vals,
                                                 unsigned int label)
{
  unsigned int unique_id = rf_nodes_.size();
  while (rf_nodes_.count(unique_id) != 0)
  {
    unique_id++;
  }
  return createNode(unique_id, feature_vals, nbr_feature_vals, label);
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
const RandomField::Node* RandomField::createNode(unsigned int node_id,
                                                 float* feature_vals,
                                                 unsigned int nbr_feature_vals,
                                                 unsigned int label)
{
  if (rf_nodes_.count(node_id) != 0)
  {
    ROS_ERROR("Cannot add node to random field b/c id %u already exists", node_id);
    return NULL;
  }

  RandomField::Node* new_node = new RandomField::Node(node_id, label);
  new_node->setFeatures(feature_vals, nbr_feature_vals);
  rf_nodes_[node_id] = new_node;
  return new_node;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
const RandomField::Clique* RandomField::createClique(unsigned int clique_set_idx, const list<
    RandomField::Node*>& nodes, float* feature_vals, unsigned int nbr_feature_vals)
{
  map<unsigned int, RandomField::Clique*>& clique_set = clique_sets_[clique_set_idx];

  unsigned int unique_id = clique_set.size();
  while (clique_set.count(unique_id) != 0)
  {
    unique_id++;
  }
  return createClique(unique_id, clique_set_idx, nodes, feature_vals, nbr_feature_vals);
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
const RandomField::Clique* RandomField::createClique(unsigned int clique_id,
                                                     unsigned int clique_set_idx,
                                                     const list<RandomField::Node*>& nodes,
                                                     float* feature_vals,
                                                     unsigned int nbr_feature_vals)
{
  map<unsigned int, RandomField::Clique*>& clique_set = clique_sets_[clique_set_idx];
  if (clique_set.count(clique_id) != 0)
  {
    ROS_ERROR("Cannot add clique to cs %u b/c id %u already exists", clique_set_idx, clique_id);
    return NULL;
  }

  // TODO need to verify that nodes are contained in this RandomField???

  RandomField::Clique* new_clique = new RandomField::Clique(clique_id);
  list<Node*>::const_iterator iter_nodes;
  for (iter_nodes = nodes.begin(); iter_nodes != nodes.end() ; iter_nodes++)
  {
    new_clique->addNode(**iter_nodes);
  }
  new_clique->setFeatures(feature_vals, nbr_feature_vals);

  clique_set[clique_id] = new_clique;
  return new_clique;
}

// -----------------------------------------------------------------------------------------------------------
// RandomField::GenericClique, RandomField::Node, RandomField::Clique definitions below
// -----------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::GenericClique::GenericClique()
{
  id_ = 0;
  feature_vals_ = NULL;
  nbr_feature_vals_ = 0;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::GenericClique::~GenericClique()
{
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::Node::Node(const unsigned int rf_id, unsigned int label)
{
  id_ = rf_id;
  label_ = label;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
RandomField::Clique::Clique(const unsigned int rf_id)
{
  id_ = rf_id;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
void RandomField::Clique::addNode(const Node& new_node)
{
  // Add node id to list
  node_ids_.push_back(new_node.getRandomFieldID());

  // Add label->node_id to mapping
  if (labels_to_node_ids_.count(new_node.getLabel()) == 0)
  {
    labels_to_node_ids_[new_node.getLabel()] = list<unsigned int> ();
  }
  labels_to_node_ids_[new_node.getLabel()].push_back(new_node.getRandomFieldID());
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
int RandomField::Clique::updateLabels(const map<unsigned int, unsigned int>& node_labels)
{
  list<unsigned int>::iterator iter_node_ids;
  unsigned int curr_node_id = 0;

#if ROS_DEBUG
  // Verify labeling contains each node id contained in this Clique
  for (iter_node_ids = node_ids_.begin(); iter_node_ids != node_ids_.end(); iter_node_ids++)
  {
    curr_node_id = *iter_node_ids;
    if (node_labels.count(curr_node_id) == 0)
    {
      ROS_ERROR("Labeling map does not contain correct node ids");
      return -1;
    }
  }
#endif

  // Update label --> node_id mapping (labels_to_node_ids_)
  labels_to_node_ids_.clear();
  unsigned int curr_label = 0;
  for (iter_node_ids = node_ids_.begin(); iter_node_ids != node_ids_.end() ; iter_node_ids++)
  {
    curr_node_id = *iter_node_ids;
    curr_label = node_labels.find(curr_node_id)->second; // labeling[curr_node_id]
    if (labels_to_node_ids_.count(curr_label) == 0)
    {
      labels_to_node_ids_[curr_label] = list<unsigned int> ();
    }
    labels_to_node_ids_[curr_label].push_back(curr_node_id);
  }
  return 0;
}

// --------------------------------------------------------------
/*! See function definition */
// --------------------------------------------------------------
int RandomField::Clique::getModeLabels(unsigned int& mode1_label,
                                       unsigned int& mode1_count,
                                       unsigned int& mode2_label,
                                       unsigned int& mode2_count,
                                       map<unsigned int, unsigned int>* tempo_labeling)
{
  mode1_label = 0;
  mode1_count = 0;
  mode2_label = 0;
  mode2_count = 0;

  map<unsigned int, list<unsigned int> > tempo_labels_to_node_ids;

  // -------------------------------------------------------
  // Determine a mapping from label->[node_ids].
  // Recalculate if using temporary label information,
  // otherwise use internal information
  map<unsigned int, list<unsigned int> >* labels_to_node_ids = NULL;
  if (tempo_labeling != NULL)
  {
    // populate temporary mapping: temporary_label --> [node ids]
    unsigned int curr_node_id = 0;
    unsigned int curr_tempo_node_label = 0;
    for (list<unsigned int>::iterator iter_node_ids = node_ids_.begin() ; iter_node_ids != node_ids_.end() ; iter_node_ids++)
    {
      curr_node_id = *iter_node_ids;

      // ---------
      // Get the node's temporary label
      if (tempo_labeling->count(curr_node_id) == 0)
      {
        ROS_ERROR("Could not find node id %u in the temporary labeling", curr_node_id);
        return -1;
      }
      curr_tempo_node_label = (*tempo_labeling)[curr_node_id];

      // ---------
      // Add node temporary label's list
      if (tempo_labels_to_node_ids.count(curr_tempo_node_label) == 0)
      {
        tempo_labels_to_node_ids[curr_tempo_node_label] = list<unsigned int> ();
      }
      tempo_labels_to_node_ids[curr_tempo_node_label].push_back(curr_node_id);
    }

    labels_to_node_ids = &tempo_labels_to_node_ids;
  }
  else
  {
    labels_to_node_ids = &labels_to_node_ids_;
  }

  // -------------------------------------------------------
  // Iterate over each label, compare each's number of associated nodes and update modes appropriately
  unsigned int curr_count = 0;
  for (map<unsigned int, list<unsigned int> >::iterator iter = labels_to_node_ids->begin() ; iter
      != labels_to_node_ids->end() ; iter++)
  {
    list<unsigned int>& curr_node_list = iter->second;
    curr_count = curr_node_list.size();

    // Update mode 1 if necessary
    if (curr_count > mode1_count)
    {
      // shift mode 1 to second place
      mode2_label = mode1_label;
      mode2_count = mode1_count;

      // update mode 1
      mode1_label = iter->first; // label
      mode1_count = curr_count;
    }
    // Update mode 2 if necessary
    else if (curr_count > mode2_count)
    {
      mode2_label = iter->first; // label
      mode2_count = curr_count;
    }
  }

  return 0;
}
