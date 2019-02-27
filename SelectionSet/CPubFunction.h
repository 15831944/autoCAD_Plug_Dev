#pragma once

wchar_t* c2w(const char *str);

class CPubFunction
{
public:
	// ����ָ���Ķ���ߴ�����Ӧ�ļ��������ߣ���������������pPline ָ����֪�Ķ���ߣ�pGeCurve ������������ļ�������
	static bool PolyToGeCurve(AcDbPolyline *&pPline, AcGeCurve2d *&pGeCurve);
	
	// ���ڸ���ָ����һ��㴴��һ���������������������һ������ arrPoints���ò���ָ��һ��㣬���������ش����Ľ������������
	static struct resbuf* BuidRbFromPtArray(AcGePoint2dArray &arrPoints);

	//	ѡ��ָ��������ڲ������������߹��ɵ������ཻ��������ʵ��
	static bool SelectEntInPoly(AcDbPolyline *pPline, AcDbObjectIdArray &ObjectIdArray, const char *selectMode, double approxEps);

	//	���浱ǰ�ӿ������ǵ������
	static bool GetViewPortBound(AcGePoint2d &ptMin, AcGePoint2d &ptMax);

	static void ZoomExtents();
	static AcDbViewTableRecord GetCurrentView();
	static Acad::ErrorStatus WindowZoom(const AcGePoint2d &ptMin, const AcGePoint2d &ptMax, double scale);
};	

bool CPubFunction::PolyToGeCurve(AcDbPolyline *&pPline, AcGeCurve2d *&pGeCurve)
{
	int nSegs; // ���߶εĶ���
	AcGeLineSeg2d line, *pLine; // �������ߵ�ֱ�߶β���
	AcGeCircArc2d arc, *pArc; // �������ߵ�Բ������
	AcGeVoidPointerArray geCurves; // ָ����ɼ������߸��ֶε�ָ������

	nSegs = pPline->numVerts() - 1;

	// ���ݶ��߶δ�����Ӧ�ķֶμ�������
	for (int i = 0; i < nSegs; i++)
	{
		if (pPline->segType(i) == AcDbPolyline::kLine)
		{
			pPline->getLineSegAt(i, line);
			pLine = new AcGeLineSeg2d(line);
			geCurves.append(pLine);
		}
		else if (pPline->segType(i) == AcDbPolyline::kArc)
		{
			pPline->getArcSegAt(i, arc);
			pArc = new AcGeCircArc2d(arc);
			geCurves.append(pArc);
		}
	}

	// ����պ϶�������һ����Բ�������
	if (pPline->isClosed() && pPline->segType(nSegs) == AcDbPolyline::kArc)
	{
		pPline->getArcSegAt(nSegs, arc);
		pArc = new AcGeCircArc2d(arc);
		pArc->setAngles(arc.startAng(), arc.endAng() - (arc.endAng() - arc.startAng()) / 100);
		geCurves.append(pArc);
	}

	// ���ݷֶεļ������ߴ�����Ӧ�ĸ�������
	if (geCurves.length() == 1)
	{
		pGeCurve = (AcGeCircArc2d *)geCurves[0];
	}
	else
	{
		pGeCurve = new AcGeCompositeCurve2d(geCurves);
	}

	// �ͷŶ�̬������ڴ�
	if (geCurves.length() > 1)
	{
		for (int i = 0; i < geCurves.length(); i++)
		{
			delete geCurves[i];
		}
	}

	return true;
}

bool CPubFunction::SelectEntInPoly(AcDbPolyline *pPline, AcDbObjectIdArray &ObjectIdArray, const char *selectMode, double approxEps)
{
	// �ж�selectMode����Ч��
	if (strcmp(selectMode, "CP") != 0 && strcmp(selectMode, "WP") != 0)
	{
		acedAlert(_T("����SelectEntInPline�У�ָ������Ч��ѡ��ģʽ"));
		return false;
	}
	
	// ������������е�ObjectId
	for (int i = 0; i < ObjectIdArray.length(); i++)
	{
		ObjectIdArray.removeAt(i);
	}

	AcGeCurve2d *pGeCurve; // ���߶ζ�Ӧ�ļ�������

	Adesk::Boolean bClosed = pPline->isClosed(); // ���߶��Ƿ�պ�

	// ȷ�����߶���Ϊѡ���ʱ�Ǳպϵ�
	if (bClosed != Adesk::kTrue)
	{
		pPline->setClosed(!bClosed);
	}

	// ������Ӧ�ļ���������
	CPubFunction::PolyToGeCurve(pPline, pGeCurve);

	AcGePoint2dArray SamplePtArray; // �洢���ߵ�������
	AcGeDoubleArray ParamArray;  // �洢�������Ӧ�Ĳ���ֵ
	AcGePoint2d ptStart, ptEnd; // �������ߵ������յ�

	Adesk::Boolean bRet = pGeCurve->hasStartPoint(ptStart);
	bRet = pGeCurve->hasEndPoint(ptStart);

	double valueSt = pGeCurve->paramOf(ptStart);
	double valueEn = pGeCurve->paramOf(ptEnd);

	pGeCurve->getSamplePoints(valueSt, valueEn, approxEps, SamplePtArray, ParamArray);

	delete pGeCurve; // �ں���PolyToGeCurve�շ������ڴ�

	// ȷ�����������ص㲻�غ�
	AcGeTol tol;
	tol.setEqualPoint(0.01);
	AcGePoint2d ptFirst = SamplePtArray[0];
	AcGePoint2d ptLast = SamplePtArray[SamplePtArray.length() - 1];
	if (ptFirst.isEqualTo(ptLast))
	{
		SamplePtArray.removeLast();
	}

	// ���������㴴���������������
	struct  resbuf *rb;
	rb = CPubFunction::BuidRbFromPtArray(SamplePtArray);

	// ʹ��acedSSGet��������ѡ��
	ads_name ssName;
	int rt = acedSSGet(c2w(selectMode), rb, NULL, NULL, ssName);

	if (rt != RTNORM)
	{
		acutRelRb(rb);	// �ͷŽ������������
		return false;
	}

	// ��ѡ�������еĶ�����ӵ�ObjectIdArray
	long length;
	acedSSLength(ssName, &length);
	for (int i = 0; i < length; i++)
	{
		// ���ָ��Ԫ�ص�ObjectId
		ads_name ent;
		acedSSName(ssName, i, ent);
		AcDbObjectId objId;
		acdbGetObjectId(objId, ent);

		// ���ָ��ǰԪ�ص�ָ��
		AcDbEntity *pEnt;
		Acad::ErrorStatus es = acdbOpenAcDbEntity(pEnt, objId, AcDb::kForRead);

		// ѡ����Ϊ�߽�Ķ��߶��ˣ�ֱ�������ô�ѭ��
		if (es == Acad::eWasOpenForWrite)
			continue;

		ObjectIdArray.append(pEnt->objectId());

		pEnt->close();
	}

	// �ͷ��ڴ�
	acutRelRb(rb);	// �ͷŽ������������
	acedSSFree(ssName); // ɾ��ѡ��
	return true;
}

struct resbuf* CPubFunction::BuidRbFromPtArray(AcGePoint2dArray &arrPoints)
{
	struct resbuf *retRb = NULL;
	int count = arrPoints.length();
	if (count <= 1)
	{
		acedAlert(_T("����BuildBbFromPtArray�У����������Ԫ�ظ������㣡"));
		return retRb;
	}

	// ʹ�õ�һ������������������������ͷ���
	ads_point adsPt;
	adsPt[X] = arrPoints[0].x;
	adsPt[Y] = arrPoints[0].y;
	retRb = acutBuildList(RTPOINT, adsPt, RTNONE);

	struct resbuf *nextRb = retRb;	// ����ָ��

	for (int i = 1; i < count; i++)	// ע�⣺�����ǵ�һ��Ԫ�أ����i��1��ʼ
	{
		adsPt[X] = arrPoints[i].x;
		adsPt[Y] = arrPoints[i].y;
		// ��̬�����µĽڵ㣬���������ӵ�ԭ��������β��
		nextRb->rbnext = acutBuildList(RTPOINT, adsPt, RTNONE);
		nextRb = nextRb->rbnext;
	}
	return retRb;
}

bool CPubFunction::GetViewPortBound(AcGePoint2d &ptMin, AcGePoint2d &ptMax)
{
	// ��õ�ǰ�ӿڵĸ߶�
	double viewHeight; // �ӿڸ߶ȣ���ͼ�ε�λ��ʾ��
	struct resbuf rbViewSize;
	if (acedGetVar(_T("VIEWSIZE"), &rbViewSize) != RTNORM)
	{
		return false;
	}

	viewHeight = rbViewSize.resval.rreal;

	// ��õ�ǰ�ӿڵĿ��
	double viewWidth;	// �ӿڿ�ȣ���ͼ�ε�λ��ʾ��
	struct resbuf rbScreenSize;
	if (acedGetVar(_T("SCREENSIZE"), &rbScreenSize) != RTNORM)
	{
		return false;
	}

	// width / height = rpoint[X] / rpoint[Y] ���ø߿�ȼ���
	viewWidth = (rbScreenSize.resval.rpoint[X] / rbScreenSize.resval.rpoint[Y]) *viewHeight;

	// ��õ�ǰ�ӿ����ĵ�WCS����
	AcGePoint3d viewCenterPt; // �ӿ�����
	struct resbuf rbViewCtr;
	if (acedGetVar(_T("VIEWCTR"), &rbViewCtr) != RTNORM)
	{
		return false;
	}

	struct resbuf UCS, WCS;
	WCS.restype = RTSHORT;
	WCS.resval.rint = 0;
	UCS.restype = RTSHORT;
	UCS.resval.rint = 1;

	acedTrans(rbViewCtr.resval.rpoint, &UCS, &WCS, 0, rbViewCtr.resval.rpoint);
	viewCenterPt = asPnt3d(rbViewCtr.resval.rpoint);

	// �����ӿڽǵ�����
	ptMin[X] = viewCenterPt[X] - viewWidth / 2;
	ptMin[Y] = viewCenterPt[Y] - viewHeight / 2;
	ptMax[X] = viewCenterPt[X] + viewWidth / 2;
	ptMax[Y] = viewCenterPt[Y] + viewHeight / 2;
	return true;
}

void CPubFunction::ZoomExtents()
{
	AcDbBlockTable *pBlkTbl;
	AcDbBlockTableRecord *pBlkTblRcd;
	acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlkTbl,AcDb::kForRead);
	
	pBlkTbl->getAt(ACDB_MODEL_SPACE, pBlkTblRcd, AcDb::kForRead);
	pBlkTbl->close();

	// ��õ�ǰͼ��������ʵ�����С��Χ��
	AcDbExtents extent;
	extent.addBlockExt(pBlkTblRcd);
	pBlkTblRcd->close();

	// ���㳤���εĶ���
	ads_point pt[7];
	pt[0][X] = pt[3][X] = pt[4][X] = pt[7][X] = extent.minPoint().x;
	pt[1][X] = pt[2][X] = pt[5][X] = pt[6][X] = extent.maxPoint().x;
	pt[0][Y] = pt[1][Y] = pt[4][Y] = pt[5][Y] = extent.minPoint().y;
	pt[2][Y] = pt[3][Y] = pt[6][Y] = pt[7][Y] = extent.maxPoint().y;
	pt[0][Z] = pt[1][Z] = pt[2][Z] = pt[3][Z] = extent.maxPoint().z;
	pt[4][Z] = pt[5][Z] = pt[6][Z] = pt[7][Z] = extent.minPoint().z;

	// ������������нǵ�ת�Ƶ�DCS��
	struct resbuf wcs, dcs; // ת������ʱʹ�õ�����ϵͳ��� 
	wcs.restype = RTSHORT;
	wcs.resval.rint = 0;
	dcs.restype = RTSHORT;

	dcs.resval.rint = 2;
	acedTrans(pt[0], &wcs, &dcs, 0, pt[0]);
	acedTrans(pt[1], &wcs, &dcs, 0, pt[1]);
	acedTrans(pt[2], &wcs, &dcs, 0, pt[2]);
	acedTrans(pt[3], &wcs, &dcs, 0, pt[3]);
	acedTrans(pt[4], &wcs, &dcs, 0, pt[4]);
	acedTrans(pt[5], &wcs, &dcs, 0, pt[5]);
	acedTrans(pt[6], &wcs, &dcs, 0, pt[6]);
	acedTrans(pt[7], &wcs, &dcs, 0, pt[7]);
	// ������нǵ���DCS����С�İ�Χ����
	double xMax = pt[0][X], xMin = pt[0][X];
	double yMax = pt[0][Y], yMin = pt[0][Y];
	for (int i = 1; i <= 7; i++)
	{
		if (pt[i][X] > xMax)
			xMax = pt[i][X];
		if (pt[i][X] < xMin)
			xMin = pt[i][X];
		if (pt[i][Y] > yMax)
			yMax = pt[i][Y];
		if (pt[i][Y] < yMin)
			yMin = pt[i][Y];
	}
	AcDbViewTableRecord view = GetCurrentView();
	// ������ͼ�����ĵ�
	view.setCenterPoint(AcGePoint2d((xMin + xMax) / 2,
		(yMin + yMax) / 2));
	// ������ͼ�ĸ߶ȺͿ��
	view.setHeight(fabs(yMax - yMin));
	view.setWidth(fabs(xMax - xMin));

	// ����ͼ��������Ϊ��ǰ��ͼ
	Acad::ErrorStatus es = acedSetCurrentView(&view, NULL);
}


AcDbViewTableRecord CPubFunction::GetCurrentView()
{
	AcDbViewTableRecord view;
	struct resbuf rb;
	struct resbuf wcs, ucs, dcs; // ת������ʱʹ�õ�����ϵͳ���
	wcs.restype = RTSHORT;
	wcs.resval.rint = 0;
	ucs.restype = RTSHORT;
	ucs.resval.rint = 1;
	dcs.restype = RTSHORT;
	dcs.resval.rint = 2;
	// ��õ�ǰ�ӿڵ�"�鿴"ģʽ
	acedGetVar(_T("VIEWMODE"), &rb);
	view.setPerspectiveEnabled(rb.resval.rint & 1);
	view.setFrontClipEnabled(rb.resval.rint & 2);
	view.setBackClipEnabled(rb.resval.rint & 4);
	view.setFrontClipAtEye(!(rb.resval.rint & 16));
	// ��ǰ�ӿ�����ͼ�����ĵ㣨UCS���꣩
	acedGetVar(_T("VIEWCTR"), &rb);
	acedTrans(rb.resval.rpoint, &ucs, &dcs, 0, rb.resval.rpoint);
	view.setCenterPoint(AcGePoint2d(rb.resval.rpoint[X],
		rb.resval.rpoint[Y]));
	// ��ǰ�ӿ�͸��ͼ�еľ�ͷ���೤�ȣ���λΪ���ף�
	acedGetVar(_T("LENSLENGTH"), &rb);
	view.setLensLength(rb.resval.rreal);
	// ��ǰ�ӿ���Ŀ����λ�ã��� UCS �����ʾ��
	acedGetVar(_T("TARGET"), &rb);
	acedTrans(rb.resval.rpoint, &ucs, &wcs, 0, rb.resval.rpoint);
	view.setTarget(AcGePoint3d(rb.resval.rpoint[X],
		rb.resval.rpoint[Y], rb.resval.rpoint[Z]));
	// ��ǰ�ӿڵĹ۲췽��UCS��
	acedGetVar(_T("VIEWDIR"), &rb);
	acedTrans(rb.resval.rpoint, &ucs, &wcs, 1, rb.resval.rpoint);
	view.setViewDirection(AcGeVector3d(rb.resval.rpoint[X],
		rb.resval.rpoint[Y], rb.resval.rpoint[Z]));
	// ��ǰ�ӿڵ���ͼ�߶ȣ�ͼ�ε�λ��
	acedGetVar(_T("VIEWSIZE"), &rb);
	view.setHeight(rb.resval.rreal);
	double height = rb.resval.rreal;
	// ������Ϊ��λ�ĵ�ǰ�ӿڵĴ�С��X �� Y ֵ��
	acedGetVar(_T("SCREENSIZE"), &rb);
	view.setWidth(rb.resval.rpoint[X] / rb.resval.rpoint[Y] * height);
	// ��ǰ�ӿڵ���ͼŤת��
	acedGetVar(_T("VIEWTWIST"), &rb);
	view.setViewTwist(rb.resval.rreal);
	// ��ģ��ѡ������һ������ѡ���Ϊ��ǰ
	acedGetVar(_T("TILEMODE"), &rb);
	int tileMode = rb.resval.rint;
	// ���õ�ǰ�ӿڵı�ʶ��
	acedGetVar(_T("CVPORT"), &rb);
	int cvport = rb.resval.rint;
	// �Ƿ���ģ�Ϳռ����ͼ
	bool paperspace = ((tileMode == 0) && (cvport == 1)) ? true : false;
	view.setIsPaperspaceView(paperspace);
	if (!paperspace)
	{
		// ��ǰ�ӿ���ǰ�����ƽ�浽Ŀ��ƽ���ƫ����
		acedGetVar(_T("FRONTZ"), &rb);
		view.setFrontClipDistance(rb.resval.rreal);
		// ��õ�ǰ�ӿں������ƽ�浽Ŀ��ƽ���ƫ��ֵ
		acedGetVar(_T("BACKZ"), &rb);
		view.setBackClipDistance(rb.resval.rreal);
	}
	else
	{
		view.setFrontClipDistance(0.0);
		view.setBackClipDistance(0.0);
	}
	return view;
}

Acad::ErrorStatus CPubFunction::WindowZoom(const AcGePoint2d &ptMin, const AcGePoint2d &ptMax, double scale)
{
	AcDbViewTableRecord view;
	AcGePoint2d ptCenter2d((ptMin[X] + ptMax[X]) / 2,
		(ptMin[Y] + ptMax[Y]) / 2);
	view.setCenterPoint(ptCenter2d);
	view.setWidth((ptMax[X] - ptMin[X]) / scale);
	view.setHeight((ptMax[Y] - ptMin[Y]) / scale);
	Acad::ErrorStatus es = acedSetCurrentView(&view, NULL);
	return es;
}

wchar_t* c2w(const char *str)
{
	int length = strlen(str) + 1;
	wchar_t *t = (wchar_t*)malloc(sizeof(wchar_t)*length);
	memset(t, 0, length * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), t, length);
	return t;
}
