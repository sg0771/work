// 运行ffmpeg.exe 等其它exe
//并且将输出结果显示在界面上

#include "stdafx.h"
#include "CFfmpeg.h"

//窗体大小改变
LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	//RECT rc;
	//::GetClientRect(m_hWnd, &rc);

	return 0;
}

//初始化对话框
LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);

	DoDataExchange(FALSE);
	return 0;
}

//水平滚动消息
LRESULT CLASS_NAME::OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	return 0;
}

//定时器消息
LRESULT CLASS_NAME::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return 0;
}

//销毁
LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	return 0;
}
