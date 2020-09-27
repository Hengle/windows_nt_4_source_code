; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CFServicePage
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "Fscfg.h"
VbxHeaderFile=Fscfg.h
VbxImplementationFile=Fscfg.cpp
LastPage=0

ClassCount=3
Class1=CFSessionsPage
Class2=CFServicePage

ResourceCount=5
Resource1=IDD_USER_SESSIONS
Resource2=IDD_DIRECTORIES
Class3=CUserSessionsDlg
Resource3=IDD_MESSAGES
Resource4=IDD_SERVICE
Resource5=IDD_DIRECTORY_PROPERTIES

[CLS:CFServicePage]
Type=0
HeaderFile=Fservic.h
ImplementationFile=Fservic.cpp
Filter=D
LastObject=IDC_EDIT_TCP_PORT
BaseClass=INetPropertyPage
VirtualFilter=idWC

[DLG:IDD_SERVICE]
Type=1
Class=CFServicePage
ControlCount=18
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_TCP_PORT,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_CONNECTION_TIMEOUT,edit,1350631552
Control5=IDC_SPIN_CONNECTION_TIMEOUT,msctls_updown32,1342177334
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_EDIT_MAX_CONNECTIONS,edit,1350631552
Control9=IDC_STATIC,button,1342177287
Control10=IDC_CHECK_ALLOW_ANONYMOUS,button,1342242819
Control11=IDC_STATIC_USERNAME,static,1342308352
Control12=IDC_EDIT_USERNAME,edit,1350631552
Control13=IDC_STATIC_PW,static,1342308352
Control14=IDC_EDIT_PASSWORD,edit,1350631584
Control15=IDC_CHECK_ONLY_ANYMOUS,button,1342242819
Control16=IDC_STATIC,static,1342308352
Control17=IDC_EDIT_COMMENT,edit,1350631552
Control18=IDC_BUTTON_CURRENT_SESSIONS,button,1342242816

[CLS:CFSessionsPage]
Type=0
HeaderFile=Fsessio.h
ImplementationFile=Fsessio.cpp
Filter=D
LastObject=IDC_BUTTON_CURRENT_SESSIONS
VirtualFilter=dWC

[DLG:IDD_USER_SESSIONS]
Type=1
Class=CUserSessionsDlg
ControlCount=10
Control1=IDC_STATIC_USERS,static,1342308352
Control2=IDC_STATIC_FROM,static,1342308352
Control3=IDC_STATIC_TIME,static,1342308352
Control4=IDC_LIST_USERS,listbox,1352728865
Control5=IDC_STATIC_NUM_CONNECTED,static,1342308352
Control6=IDC_BUTTON_DISCONNECT,button,1342242816
Control7=IDC_BUTTON_DISCONNECT_ALL,button,1342242816
Control8=IDOK,button,1342242817
Control9=IDC_BUTTON_REFRESH,button,1342242816
Control10=ID_HELP,button,1342242816

[CLS:CUserSessionsDlg]
Type=0
HeaderFile=usersess.h
ImplementationFile=usersess.cpp
LastObject=IDC_STATIC_NUM_CONNECTED
Filter=D
VirtualFilter=dWC

[DLG:IDD_DIRECTORIES]
Type=1
ControlCount=8
Control1=IDC_LIST_DIRECTORIES,listbox,1352729889
Control2=IDC_ADD,button,1342242816
Control3=IDC_REMOVE,button,1342242816
Control4=IDC_BUTTON_EDIT,button,1342242816
Control5=IDC_STATIC,button,1342177287
Control6=IDC_RADIO_UNIX,button,1342373897
Control7=IDC_RADIO_MSDOS,button,1342177289
Control8=IDC_STATIC,static,1342308356

[DLG:IDD_MESSAGES]
Type=1
ControlCount=6
Control1=65535,static,1342308352
Control2=IDC_EDIT_WELCOME,edit,1352732868
Control3=65535,static,1342308352
Control4=IDC_EDIT_EXIT,edit,1350631552
Control5=65535,static,1342308352
Control6=IDC_EDIT_MAX_CONNECTIONS,edit,1350631552

[DLG:IDD_DIRECTORY_PROPERTIES]
Type=1
ControlCount=27
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_DIRECTORY,edit,1350631552
Control3=IDC_BUTTON_BROWSE,button,1342242816
Control4=IDC_STATIC,button,1342177287
Control5=IDC_RADIO_HOME,button,1342308361
Control6=IDC_RADIO_ALIAS,button,1342177289
Control7=IDC_STATIC,static,1342308355
Control8=IDC_STATIC_ALIAS,static,1342308352
Control9=IDC_EDIT_ALIAS,edit,1350631552
Control10=IDC_GROUP_ACCOUNT,button,1342177287
Control11=IDC_STATIC_USER_NAME,static,1342308352
Control12=IDC_EDIT_USER_NAME,edit,1350631552
Control13=IDC_STATIC_PASSWORD,static,1342308352
Control14=IDC_PASSWORD,edit,1350631584
Control15=IDC_GROUP_ACCESS,button,1342177287
Control16=IDC_CHECK_READ,button,1342242819
Control17=IDC_CHECK_WRITE,button,1342242819
Control18=IDC_STATIC,static,1342177284
Control19=IDOK,button,1342242817
Control20=IDCANCEL,button,1342242816
Control21=ID_HELP,button,1342242816
Control22=IDC_STATIC_IP_GROUP,button,1073741831
Control23=IDC_CHECK_USE_IP,button,1073807363
Control24=IDC_STATIC_IP_PROMPT,static,1073872896
Control25=IDC_IPA_IPADDRESS,IPAddress,1073807360
Control26=IDC_CHECK_EXECUTE,button,1073807363
Control27=IDC_CHECK_SSL,button,1073807363

