#pragma once

#include "colorlist.h"

// CSearchDlg dialog

#define BOX_FILE    5
#define BOX_NAME    6
#define BOX_MODE    7

struct SearchStruct
{
  int nfunc;
  wchar_t ntext[256];
  int ffunc;
  char ftext[256];
  int mfunc;
  char mtext[256];
  int pmode[5];
  wchar_t pname[5][256];
  int phero[5];
  int numa, numb;
  int vera, verb;
  int pata, patb;
  unsigned long lena, lenb;
  __int64 sava, savb;
};

class CSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CSearchDlg)

public:
	CSearchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSearchDlg();

  SearchStruct ss;

// Dialog Data
	enum { IDD = IDD_SEARCHOPTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  CComboBox mode[8];
  CColorBox hero[5];
  CDateTimeCtrl dfrom, dto;
  CDateTimeCtrl lfrom, lto;

  int selhero[5];

  virtual BOOL OnInitDialog ();
  afx_msg void OnDestroy ();

  virtual void OnOK ();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnCbnSelchangePhero1();
  afx_msg void OnCbnSelchangePhero2();
  afx_msg void OnCbnSelchangePhero3();
  afx_msg void OnCbnSelchangePhero4();
  afx_msg void OnCbnSelchangePhero5();
};

bool matches (char const* file, SearchStruct* ss);
