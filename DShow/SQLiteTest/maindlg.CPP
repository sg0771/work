#include "StdAfx.h"
#include "MainDlg.h"


LRESULT CMainDlg::OnBnClickedOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	int iOpen= sqlite3_open("test.db",  &m_db);//打开指定数据库
	if(m_db == NULL)
	{
		Log("Open test.db error");
		return -1;
	}
	else
	{
		Log("Open test.db OK");
		//添加一个表 schoolTable 到数据库
		int ret = sqlite3_exec(m_db, 
			"create table if not exists schoolTable(schID integer default 0, schName varchar default 'hz');",0,0,0);

	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_db)
	{
		sqlite3_close(m_db);
		m_db = NULL;
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码

	return 0;
}
