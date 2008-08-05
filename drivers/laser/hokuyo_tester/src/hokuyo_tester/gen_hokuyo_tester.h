///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __gen_hokuyo_tester__
#define __gen_hokuyo_tester__

#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/statbox.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class GenHokuyoTester
///////////////////////////////////////////////////////////////////////////////
class GenHokuyoTester : public wxPanel 
{
	private:
	
	protected:
		wxPanel* visPanel;
		wxStaticText* m_staticText2;
		wxTextCtrl* port;
		wxButton* connectButton;
		wxButton* disconnectButton;
		wxToggleButton* scanButton;
		wxTextCtrl* logText;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnConnect( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDisconnect( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnScan( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GenHokuyoTester( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~GenHokuyoTester();
	
};

#endif //__gen_hokuyo_tester__
