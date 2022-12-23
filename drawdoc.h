// drawdoc.h : interface of the CDrawDoc class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "drawobj.h"
#include "summinfo.h"

class CDrawView;

/////////////////////////////////////////////////////////////////////////////
// CDrawDoc

class CDrawDoc : public COleDocument
{
protected: // create from serialization only
	CDrawDoc();
	DECLARE_DYNCREATE(CDrawDoc)

// Attributes
public:
	CDrawObjList* GetObjects();
	const CSize& GetSize() const;
	void ComputePageSize();
	int GetMapMode() const;
	COLORREF GetPaperColor() const;
	CSummInfo* m_pSummInfo;

// Operations
public:
	CDrawObj* ObjectAt(const CPoint& point);
	void Draw(CDC* pDC, CDrawView* pView);
	void Add(CDrawObj* pObj);
	void Remove(CDrawObj* pObj);

// Implementation
public:
	virtual ~CDrawDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);

	CDrawObjList m_objects;
	CSize m_size;
	int m_nMapMode;
	COLORREF m_paperColor;

// Generated message map functions
protected:
	//{{AFX_MSG(CDrawDoc)
	afx_msg void OnViewPaperColor();
	afx_msg void OnFileSummaryInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Inlines

inline CDrawObjList* CDrawDoc::GetObjects()
	{ return &m_objects; }

inline const CSize& CDrawDoc::GetSize() const
	{ return m_size; }

inline int CDrawDoc::GetMapMode() const
	{ return m_nMapMode; }

inline COLORREF CDrawDoc::GetPaperColor() const
	{ return m_paperColor; }

/////////////////////////////////////////////////////////////////////////////
