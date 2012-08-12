#pragma once


// CChatFilters dialog

class CChatFilters : public CDialog
{
	DECLARE_DYNAMIC(CChatFilters)

  int filters;
  bool ladder;

public:
	CChatFilters(int chatFilters, bool isLadder, CWnd* pParent = NULL);   // standard constructor
	virtual ~CChatFilters();

  int getFilters () const
  {
    return filters;
  }

  static bool isMessageAllowed (int filter, unsigned long mode, unsigned long notifyType);
  static bool shouldInsertBlanks (int filter);
  static int getDefaultFilter ();

// Dialog Data
	enum { IDD = IDD_CHATFILTERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog ();

	DECLARE_MESSAGE_MAP()
public:
  int getGroupState (int group);
  void onClickItem (int group);
  void onClickHeader (int group);
  afx_msg void OnBnClickedOk();
  afx_msg void UpdateGroup1();
  afx_msg void UpdateGroup2();
  afx_msg void OnBnClickedChatMessages();
  afx_msg void OnBnClickedGameMessages();
};
