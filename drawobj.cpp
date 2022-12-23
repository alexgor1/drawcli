// drawobj.cpp - implementation for drawing objects
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include "stdafx.h"
#include "drawcli.h"

#include "drawdoc.h"
#include "drawvw.h"
#include "drawobj.h"

#include "cntritem.h"
#include "rectdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CDrawObj

IMPLEMENT_SERIAL(CDrawObj, CObject, 0)

CDrawObj::CDrawObj()
{
}

CDrawObj::~CDrawObj()
{
}

CDrawObj::CDrawObj(const CRect& rectPos)
{
	m_rectPos = rectPos;
	m_pDocument = NULL;
	m_lastSnap = NULL;
	m_snappedHandle = 0;

	m_bPen = TRUE;
	m_logpen.lopnStyle = PS_INSIDEFRAME;
	m_logpen.lopnWidth.x = 1;
	m_logpen.lopnWidth.y = 1;
	m_logpen.lopnColor = RGB(0, 0, 0);

	m_bBrush = TRUE;
	m_logbrush.lbStyle = BS_SOLID;
	m_logbrush.lbColor = RGB(192, 192, 192);
	m_logbrush.lbHatch = HS_HORIZONTAL;
}

void CDrawObj::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);

	if (ar.IsStoring())
	{
		ar << m_rectPos;
		ar << (WORD)m_bPen;
		ar.Write(&m_logpen, sizeof(LOGPEN));
		ar << (WORD)m_bBrush;
		ar.Write(&m_logbrush, sizeof(LOGBRUSH));
	}
	else
	{
		// get the document back pointer from the archive
		m_pDocument = (CDrawDoc*)ar.m_pDocument;
		ASSERT_VALID(m_pDocument);
		ASSERT_KINDOF(CDrawDoc, m_pDocument);

		WORD wTemp;
		ar >> m_rectPos;
		ar >> wTemp; m_bPen = (BOOL)wTemp;
		ar.Read(&m_logpen,sizeof(LOGPEN));
		ar >> wTemp; m_bBrush = (BOOL)wTemp;
		ar.Read(&m_logbrush, sizeof(LOGBRUSH));
	}
}

void CDrawObj::Remove()
{
	delete this;
}

void CDrawObj::Draw(CDC*)
{
}

void CDrawObj::DrawTracker(CDC* pDC, TrackerState state)
{
	ASSERT_VALID(this);

	switch (state)
	{
	case normal:
		break;

	case selected:
	case active:
		{
			int nHandleCount = GetHandleCount();
			for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1)
			{
				CPoint handle = GetHandle(nHandle);
				pDC->PatBlt(handle.x - 3, handle.y - 3, 7, 7, DSTINVERT);
			}
		}
		break;
	}
}

void CDrawObj::DrawSnapping(CDC* pDC)
{
	if (m_lastSnap)
	{
		CPoint handle = GetHandle(m_snappedHandle);
		if (*m_lastSnap != handle)
		{
			LOGPEN logpen = m_logpen;
			logpen.lopnStyle = PS_DOT;
			CPen pen;
			if (!pen.CreatePenIndirect(&logpen))
				return;
			pDC->SelectObject(&pen);
			pDC->MoveTo(handle);
			pDC->LineTo(*m_lastSnap);
		}
	}
}

// position is in logical
void CDrawObj::MoveTo(const CRect& rectPos, CDrawView* pView)
{
	ASSERT_VALID(this);

	if (rectPos == m_rectPos)
		return;

	if (pView == NULL)
	{
		Invalidate();
		m_rectPos = rectPos;
		Invalidate();
	}
	else
	{
		pView->InvalObj(this);
		m_rectPos = rectPos;
		pView->InvalObj(this);
	}

	m_pDocument->SetModifiedFlag();
}

// Note: if bSelected, hit-codes start at one for the top-left
// and increment clockwise, 0 means no hit.
// If !bSelected, 0 = no hit, 1 = hit (anywhere)

// point is in logical coordinates
int CDrawObj::HitTest(CPoint point, CDrawView* pView, BOOL bSelected)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	if (bSelected)
	{
		int nHandleCount = GetHandleCount();
		for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1)
		{
			// GetHandleRect returns in logical coords
			CRect rc = GetHandleRect(nHandle,pView);
			if (point.x >= rc.left && point.x < rc.right &&
				point.y <= rc.top && point.y > rc.bottom)
			{
				return nHandle;
			}
		}
	}
	else
	{
		if (point.x >= m_rectPos.left && point.x < m_rectPos.right &&
			point.y <= m_rectPos.top && point.y > m_rectPos.bottom)
		{
			return 1;
		}
	}

	return 0;
}

// rect must be in logical coordinates
BOOL CDrawObj::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);

	CRect rectPos = m_rectPos;
	rectPos.NormalizeRect();

	CRect rectT = rect;
	rectT.NormalizeRect();

	return !(rectT & rectPos).IsRectEmpty();
}

int CDrawObj::GetHandleCount()
{
	ASSERT_VALID(this);
	return 8;
}

// returns logical coords of center of handle
CPoint CDrawObj::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	// this gets the center regardless of left/right and top/bottom ordering
	int xCenter = m_rectPos.left + m_rectPos.Width() / 2;
	int yCenter = m_rectPos.top + m_rectPos.Height() / 2;

	int x = 0;
	int y = 0;

	switch (nHandle)
	{
	case 1:
		x = m_rectPos.left;
		y = m_rectPos.top;
		break;

	case 2:
		x = xCenter;
		y = m_rectPos.top;
		break;

	case 3:
		x = m_rectPos.right;
		y = m_rectPos.top;
		break;

	case 4:
		x = m_rectPos.right;
		y = yCenter;
		break;

	case 5:
		x = m_rectPos.right;
		y = m_rectPos.bottom;
		break;

	case 6:
		x = xCenter;
		y = m_rectPos.bottom;
		break;

	case 7:
		x = m_rectPos.left;
		y = m_rectPos.bottom;
		break;

	case 8:
		x = m_rectPos.left;
		y = yCenter;
		break;

	default:
		ASSERT(FALSE);
	}

	return CPoint(x, y);
}

// return rectange of handle in logical coords
CRect CDrawObj::GetHandleRect(int nHandleID, CDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	CRect rect;
	// get the center of the handle in logical coords
	CPoint point = GetHandle(nHandleID);
	// convert to client/device coords
	pView->DocToClient(point);
	// return CRect of handle in device coords
	rect.SetRect(point.x - 3, point.y - 3, point.x + 3, point.y + 3);
	pView->ClientToDoc(rect);

	return rect;
}

HCURSOR CDrawObj::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	LPCTSTR lpszCursor = NULL;
	switch (nHandle)
	{
	case 1:
	case 5:
		lpszCursor = IDC_SIZENWSE;
		break;

	case 2:
	case 6:
		lpszCursor = IDC_SIZENS;
		break;

	case 3:
	case 7:
		lpszCursor = IDC_SIZENESW;
		break;

	case 4:
	case 8:
		lpszCursor = IDC_SIZEWE;
		break;

	default:
		ASSERT(FALSE);
	}

	return AfxGetApp()->LoadStandardCursor(lpszCursor);
}

// point must be in logical
void CDrawObj::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView)
{
	ASSERT_VALID(this);

	CRect rectPos = m_rectPos;
	switch (nHandle)
	{
	case 1:
		rectPos.left = point.x;
		rectPos.top = point.y;
		break;

	case 2:
		rectPos.top = point.y;
		break;

	case 3:
		rectPos.right = point.x;
		rectPos.top = point.y;
		break;

	case 4:
		rectPos.right = point.x;
		break;

	case 5:
		rectPos.right = point.x;
		rectPos.bottom = point.y;
		break;

	case 6:
		rectPos.bottom = point.y;
		break;

	case 7:
		rectPos.left = point.x;
		rectPos.bottom = point.y;
		break;

	case 8:
		rectPos.left = point.x;
		break;

	default:
		ASSERT(FALSE);
	}

	MoveTo(rectPos, pView);
}

void CDrawObj::Invalidate()
{
	ASSERT_VALID(this);
	m_pDocument->UpdateAllViews(NULL, HINT_UPDATE_DRAWOBJ, this);
}

CDrawObj* CDrawObj::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawObj* pClone = new CDrawObj(m_rectPos);
	pClone->m_bPen     = m_bPen;
	pClone->m_logpen   = m_logpen;
	pClone->m_bBrush   = m_bBrush;
	pClone->m_logbrush = m_logbrush;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	return pClone;
}

void CDrawObj::OnEditProperties()
{
	ASSERT_VALID(this);

	CPropertySheet sheet(_T("Shape Properties"));
	CRectDlg pageRect;
	pageRect.m_bNoFill = !m_bBrush;
	pageRect.m_penSize = m_bPen ? m_logpen.lopnWidth.x : 0;
	sheet.AddPage(&pageRect);

	if (sheet.DoModal() != IDOK)
		return;

	m_bBrush = !pageRect.m_bNoFill;
	m_bPen = pageRect.m_penSize > 0;
	if (m_bPen)
	{
		m_logpen.lopnWidth.x = pageRect.m_penSize;
		m_logpen.lopnWidth.y = pageRect.m_penSize;
	}

	Invalidate();
	m_pDocument->SetModifiedFlag();
}

void CDrawObj::OnOpen(CDrawView* /*pView*/)
{
	OnEditProperties();
}

void CDrawObj::SetLineColor(COLORREF cr)
{
	ASSERT_VALID(this);

	m_logpen.lopnColor = cr;
	
	Invalidate();
	m_pDocument->SetModifiedFlag();
}

void CDrawObj::SetFillColor(COLORREF cr)
{
	ASSERT_VALID(this);

	m_logbrush.lbColor = cr;
	
	Invalidate();
	m_pDocument->SetModifiedFlag();
}

#ifdef _DEBUG
void CDrawObj::AssertValid()
{
	ASSERT(m_rectPos.left <= m_rectPos.right);
	ASSERT(m_rectPos.bottom <= m_rectPos.top);
}
#endif

////////////////////////////////////////////////////////////////////////////
// CDrawRect

IMPLEMENT_SERIAL(CDrawRect, CDrawObj, 0)

CDrawRect::CDrawRect()
{
}

CDrawRect::CDrawRect(const CRect& rectPos) :
	CDrawObj(rectPos)
{
	ASSERT_VALID(this);

	m_nShape = rectangle;

	m_ptRoundness.x = 16;
	m_ptRoundness.y = 16;
}

void CDrawRect::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << (WORD) m_nShape;
		ar << m_ptRoundness;
	}
	else
	{
		WORD wTemp;
		ar >> wTemp; m_nShape = (Shape)wTemp;
		ar >> m_ptRoundness;
	}
}

void CDrawRect::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CBrush brush;
	if (!brush.CreateBrushIndirect(&m_logbrush))
		return;
	CPen pen;
	if (!pen.CreatePenIndirect(&m_logpen))
		return;

	CBrush* pOldBrush;
	if (m_bBrush)
		pOldBrush = pDC->SelectObject(&brush);
	else
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);

	CPen* pOldPen;
	if (m_bPen)
		pOldPen = pDC->SelectObject(&pen);
	else
		pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);

	CRect rect = m_rectPos;
	switch (m_nShape)
	{
	case rectangle:
		pDC->Rectangle(rect);
		DrawSnapping(pDC);
		break;

	case roundRectangle:
		pDC->RoundRect(rect, m_ptRoundness);
		break;

	case ellipse:
		pDC->Ellipse(rect);
		break;

	case line:
		if (rect.top > rect.bottom)
		{
			rect.top -= m_logpen.lopnWidth.y / 2;
			rect.bottom += (m_logpen.lopnWidth.y + 1) / 2;
		}
		else
		{
			rect.top += (m_logpen.lopnWidth.y + 1) / 2;
			rect.bottom -= m_logpen.lopnWidth.y / 2;
		}

		if (rect.left > rect.right)
		{
			rect.left -= m_logpen.lopnWidth.x / 2;
			rect.right += (m_logpen.lopnWidth.x + 1) / 2;
		}
		else
		{
			rect.left += (m_logpen.lopnWidth.x + 1) / 2;
			rect.right -= m_logpen.lopnWidth.x / 2;
		}

		pDC->MoveTo(rect.TopLeft());
		pDC->LineTo(rect.BottomRight());
		DrawSnapping(pDC);
		break;
	}

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

int CDrawRect::GetHandleCount()
{
	ASSERT_VALID(this);

	return m_nShape == line ? 2 :
		CDrawObj::GetHandleCount() + (m_nShape == roundRectangle);
}

// returns center of handle in logical coordinates
CPoint CDrawRect::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	if (m_nShape == line && nHandle == 2)
	{
		nHandle = 5;
	}
	else if (m_nShape == roundRectangle && nHandle == 9)
	{
		CRect rect = m_rectPos;
		rect.NormalizeRect();
		
		CPoint point = rect.BottomRight();
		point.x -= m_ptRoundness.x / 2;
		point.y -= m_ptRoundness.y / 2;
		
		return point;
	}

	return CDrawObj::GetHandle(nHandle);
}

HCURSOR CDrawRect::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	if (m_nShape == line && nHandle == 2)
		nHandle = 5;
	else if (m_nShape == roundRectangle && nHandle == 9)
		return AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
	
	return CDrawObj::GetHandleCursor(nHandle);
}

BOOL CDrawRect::canSnap(int nHandleID) 
{
	switch (m_nShape)
	{
	case CDrawRect::rectangle:
		return nHandleID & 1;
	case CDrawRect::line:
		return TRUE;
	}
	return FALSE;
}

// point is in logical coordinates
void CDrawRect::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView)
{
	ASSERT_VALID(this);

	if (m_nShape == line && nHandle == 2)
	{
		nHandle = 5;
	}
	else if (m_nShape == roundRectangle && nHandle == 9)
	{
		CRect rect = m_rectPos;
		rect.NormalizeRect();

		if (point.x > rect.right - 1)
			point.x = rect.right - 1;
		else if (point.x < rect.left + rect.Width() / 2)
			point.x = rect.left + rect.Width() / 2;
		if (point.y > rect.bottom - 1)
			point.y = rect.bottom - 1;
		else if (point.y < rect.top + rect.Height() / 2)
			point.y = rect.top + rect.Height() / 2;
		
		m_ptRoundness.x = 2 * (rect.right - point.x);
		m_ptRoundness.y = 2 * (rect.bottom - point.y);
		
		m_pDocument->SetModifiedFlag();
		
		if (pView == NULL)
			Invalidate();
		else
			pView->InvalObj(this);

		return;
	}

	CDrawObj::MoveHandleTo(nHandle, point, pView);
}

// rect must be in logical coordinates
BOOL CDrawRect::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);

	CRect rectT = rect;
	rectT.NormalizeRect();

	CRect rectPos = m_rectPos;
	rectPos.NormalizeRect();
	if ((rectT & rectPos).IsRectEmpty())
		return FALSE;

	CRgn rgn;
	switch (m_nShape)
	{
	case rectangle:
		return TRUE;

	case roundRectangle:
		rgn.CreateRoundRectRgn(
			rectPos.left, rectPos.top,
			rectPos.right, rectPos.bottom,
			m_ptRoundness.x, m_ptRoundness.y);
		break;

	case ellipse:
		rgn.CreateEllipticRgnIndirect(rectPos);
		break;

	case line:
		{
			int x = (m_logpen.lopnWidth.x + 5) / 2;
			int y = (m_logpen.lopnWidth.y + 5) / 2;
			
			POINT points[4];
			points[0].x = rectPos.left;
			points[0].y = rectPos.top;
			points[1].x = rectPos.left;
			points[1].y = rectPos.top;
			points[2].x = rectPos.right;
			points[2].y = rectPos.bottom;
			points[3].x = rectPos.right;
			points[3].y = rectPos.bottom;

			if (rectPos.left < rectPos.right)
			{
				points[0].x -= x;
				points[1].x += x;
				points[2].x += x;
				points[3].x -= x;
			}
			else
			{
				points[0].x += x;
				points[1].x -= x;
				points[2].x -= x;
				points[3].x += x;
			}

			if (rectPos.top < rectPos.bottom)
			{
				points[0].y -= y;
				points[1].y += y;
				points[2].y += y;
				points[3].y -= y;
			}
			else
			{
				points[0].y += y;
				points[1].y -= y;
				points[2].y -= y;
				points[3].y += y;
			}
			
			rgn.CreatePolygonRgn(points, 4, ALTERNATE);
		}
		break;
	}

	return rgn.RectInRegion(rectPos);
}

CDrawObj* CDrawRect::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawRect* pClone = new CDrawRect(m_rectPos);
	pClone->m_bPen      = m_bPen;
	pClone->m_logpen    = m_logpen;
	pClone->m_bBrush    = m_bBrush;
	pClone->m_logbrush  = m_logbrush;
	pClone->m_nShape    = m_nShape;
	pClone->m_ptRoundness = m_ptRoundness;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);

	return pClone;
}

////////////////////////////////////////////////////////////////////////////
// CDrawPoly

IMPLEMENT_SERIAL(CDrawPoly, CDrawObj, 0)

CDrawPoly::CDrawPoly()
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
}

CDrawPoly::CDrawPoly(const CRect& rectPos) :
	CDrawObj(rectPos)
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
	m_bPen = TRUE;
	m_bBrush = FALSE;
}

CDrawPoly::~CDrawPoly()
{
	if (m_points != NULL)
		delete[] m_points;
}

void CDrawPoly::Serialize(CArchive& ar)
{
	CDrawObj::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << (WORD) m_nPoints;
		ar << (WORD) m_nAllocPoints;
		for (int i = 0; i< m_nPoints; i++)
			ar << m_points[i];
	}
	else
	{
		WORD wTemp;
		ar >> wTemp; m_nPoints = wTemp;
		ar >> wTemp; m_nAllocPoints = wTemp;
		m_points = new CPoint[m_nAllocPoints];
		for (int i = 0; i < m_nPoints; i++)
			ar >> m_points[i];
	}
}

void CDrawPoly::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CBrush brush;
	if (!brush.CreateBrushIndirect(&m_logbrush))
		return;

	CPen pen;
	if (!pen.CreatePenIndirect(&m_logpen))
		return;

	CBrush* pOldBrush;
	if (m_bBrush)
		pOldBrush = pDC->SelectObject(&brush);
	else
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);

	CPen* pOldPen;
	if (m_bPen)
		pOldPen = pDC->SelectObject(&pen);
	else
		pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);

	pDC->Polygon(m_points, m_nPoints);

	DrawSnapping(pDC);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

// position must be in logical coordinates
void CDrawPoly::MoveTo(const CRect& rectPos, CDrawView* pView)
{
	ASSERT_VALID(this);
	
	if (rectPos == m_rectPos)
		return;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	for (int i = 0; i < m_nPoints; i += 1)
	{
		m_points[i].x += rectPos.left - m_rectPos.left;
		m_points[i].y += rectPos.top - m_rectPos.top;
	}

	m_rectPos = rectPos;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);
	
	m_pDocument->SetModifiedFlag();
}

int CDrawPoly::GetHandleCount()
{
	return m_nPoints;
}

CPoint CDrawPoly::GetHandle(int nHandle)
{
	ASSERT_VALID(this);
	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	
	return m_points[nHandle - 1];
}

HCURSOR CDrawPoly::GetHandleCursor(int)
{
	return AfxGetApp()->LoadStandardCursor(IDC_ARROW);
}

// point is in logical coordinates
void CDrawPoly::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	
	if (m_points[nHandle - 1] == point)
		return;

	m_points[nHandle - 1] = point;
	RecalcBounds(pView);

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);
	
	m_pDocument->SetModifiedFlag();
}

// rect must be in logical coordinates
BOOL CDrawPoly::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);
	
	CRgn rgn;
	rgn.CreatePolygonRgn(m_points, m_nPoints, ALTERNATE);
	
	return rgn.RectInRegion(rect);
}

CDrawObj* CDrawPoly::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawPoly* pClone = new CDrawPoly(m_rectPos);
	pClone->m_bPen         = m_bPen;
	pClone->m_logpen       = m_logpen;
	pClone->m_bBrush       = m_bBrush;
	pClone->m_logbrush     = m_logbrush;
	pClone->m_points = new CPoint[m_nAllocPoints];
	memcpy(pClone->m_points, m_points, sizeof(CPoint) * m_nPoints);
	pClone->m_nAllocPoints = m_nAllocPoints;
	pClone->m_nPoints      = m_nPoints;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	
	return pClone;
}

// point is in logical coordinates
void CDrawPoly::AddPoint(const CPoint& point, CDrawView* pView)
{
	ASSERT_VALID(this);
	
	if (m_nPoints == m_nAllocPoints)
	{
		CPoint* newPoints = new CPoint[m_nAllocPoints + 10];
		if (m_points != NULL)
		{
			memcpy(newPoints, m_points, sizeof(CPoint) * m_nAllocPoints);
			delete[] m_points;
		}
		m_points = newPoints;
		m_nAllocPoints += 10;
	}

	if (m_nPoints == 0 || m_points[m_nPoints - 1] != point)
	{
		m_points[m_nPoints++] = point;
		if (!RecalcBounds(pView))
		{
			if (pView == NULL)
				Invalidate();
			else
				pView->InvalObj(this);
		}

		m_pDocument->SetModifiedFlag();
	}
}

BOOL CDrawPoly::RecalcBounds(CDrawView* pView)
{
	ASSERT_VALID(this);

	if (m_nPoints == 0)
		return FALSE;

	CRect bounds(m_points[0], CSize(0, 0));
	for (int i = 1; i < m_nPoints; ++i)
	{
		if (m_points[i].x < bounds.left)
			bounds.left = m_points[i].x;
		if (m_points[i].x > bounds.right)
			bounds.right = m_points[i].x;
		if (m_points[i].y < bounds.top)
			bounds.top = m_points[i].y;
		if (m_points[i].y > bounds.bottom)
			bounds.bottom = m_points[i].y;
	}

	if (bounds == m_rectPos)
		return FALSE;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	m_rectPos = bounds;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SERIAL(CDrawOleObj, CDrawObj, 0)

BOOL CDrawOleObj::c_bShowItems = TRUE;

CDrawOleObj::CDrawOleObj()
{
	m_extent = CSize(0,0);

	m_pClientItem = NULL;
}

CDrawOleObj::CDrawOleObj(const CRect& rectPos) :
	CDrawObj(rectPos)
{
	m_extent = CSize(0, 0);

	m_pClientItem = NULL;
}

CDrawOleObj::~CDrawOleObj()
{
	if (m_pClientItem != NULL)
	{
		m_pClientItem->Release();
		m_pClientItem = NULL;
	}
}

void CDrawOleObj::Remove()
{
	if (m_pClientItem != NULL)
	{
		m_pClientItem->Delete();
		m_pClientItem = NULL;
	}
	CDrawObj::Remove();
}

void CDrawOleObj::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);

	if (ar.IsStoring())
	{
		ar << m_extent;
		ar << m_pClientItem;
	}
	else
	{
		ar >> m_extent;
		ar >> m_pClientItem;
		m_pClientItem->m_pDrawObj = this;
	}
}

CDrawObj* CDrawOleObj::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	AfxGetApp()->BeginWaitCursor();

	CDrawOleObj* pClone = NULL;
	CDrawItem* pItem = NULL;
	TRY
	{
		// perform a "deep copy" -- need to copy CDrawOleObj and the CDrawItem
		//  that it points to.
		pClone = new CDrawOleObj(m_rectPos);
		pItem = new CDrawItem(m_pDocument, pClone);
		if (!pItem->CreateCloneFrom(m_pClientItem))
			AfxThrowMemoryException();

		pClone->m_pClientItem = pItem;
		pClone->m_bPen = m_bPen;
		pClone->m_logpen = m_logpen;
		pClone->m_bBrush = m_bBrush;
		pClone->m_logbrush = m_logbrush;
		ASSERT_VALID(pClone);

		if (pDoc != NULL)
			pDoc->Add(pClone);
	}
	CATCH_ALL(e)
	{
		if (pItem != NULL)
			pItem->Delete();

		if (pClone != NULL)
		{
			pClone->m_pClientItem = NULL;
			pClone->Remove();
		}

		AfxGetApp()->EndWaitCursor();

		THROW_LAST();
	}
	END_CATCH_ALL

	AfxGetApp()->EndWaitCursor();
	
	return pClone;
}

void CDrawOleObj::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CDrawItem* pItem = m_pClientItem;
	if (pItem != NULL)
	{
		// draw the OLE item itself
		pItem->Draw(pDC, m_rectPos);

		// don't draw tracker in print preview or on printer
		if (!pDC->IsPrinting())
		{
			// use a CRectTracker to draw the standard effects
			CRectTracker tracker;
			tracker.m_rect = m_rectPos;
			pDC->LPtoDP(tracker.m_rect);

			if (c_bShowItems)
			{
				// put correct border depending on item type
				if (pItem->GetType() == OT_LINK)
					tracker.m_nStyle |= CRectTracker::dottedLine;
				else
					tracker.m_nStyle |= CRectTracker::solidLine;
			}

			// put hatching over the item if it is currently open
			if (pItem->GetItemState() == COleClientItem::openState ||
				pItem->GetItemState() == COleClientItem::activeUIState)
			{
				tracker.m_nStyle |= CRectTracker::hatchInside;
			}

			tracker.Draw(pDC);
		}
	}
}

void CDrawOleObj::OnOpen(CDrawView* pView)
{
	CWaitCursor wait;
	
	m_pClientItem->DoVerb(
		GetKeyState(VK_CONTROL) < 0 ? OLEIVERB_OPEN : OLEIVERB_PRIMARY,
		pView);
}

void CDrawOleObj::OnEditProperties()
{
	// using COlePropertiesDialog directly means no scaling
	COlePropertiesDialog dlg(m_pClientItem, 100, 100, NULL);

	dlg.DoModal();
}

// position is in logical
void CDrawOleObj::MoveTo(const CRect& rectPos, CDrawView* pView)
{
	ASSERT_VALID(this);

	if (rectPos == m_rectPos)
		return;

	// call base class to update position
	CDrawObj::MoveTo(rectPos, pView);

	// update position of in-place editing session on position change
	if (m_pClientItem->IsInPlaceActive())
		m_pClientItem->SetItemRects();
}

/////////////////////////////////////////////////////////////////////////////
