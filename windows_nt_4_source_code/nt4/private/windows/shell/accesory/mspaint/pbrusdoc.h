 // pbrusdoc.h : interface of the CPBDoc class
//
/***************************************************************************/

class CPBSrvrItem;
class CBitmapObj;

class CPBDoc : public COleServerDoc
    {
    protected: // create from serialization only

    CPBDoc();

    DECLARE_DYNCREATE( CPBDoc )

    // Attributes

    public:

    CBitmapObj* m_pBitmapObj;
    CBitmapObj* m_pBitmapObjNew;
    CString     m_sName;
    BOOL        m_bNewDoc;
    BOOL        m_bObjectLoaded;
    BOOL        m_bPaintFormat;
    BOOL        m_bNonBitmapFile;   // TRUE if we loaded a non .BMP file.

    // Operations
    public:

    CPBSrvrItem* GetEmbeddedItem()
                    { return (CPBSrvrItem*)COleServerDoc::GetEmbeddedItem(); }
    BOOL CreateNewDocument();
    BOOL SaveTheDocument();
    BOOL Finish();

    void OLESerialize(CArchive& ar);   // overridden for document i/o
    BOOL SerializeBitmap(CArchive& ar, CBitmapObj* pBitmapCur,
        CBitmapObj* pBitmapNew, BOOL bOLEObject);

    // Implementation
    public:

    virtual ~CPBDoc();
    virtual void Serialize(CArchive& ar);   // overridden for document i/o

    virtual void SaveToStorage(CObject* pObject);
    virtual void LoadFromStorage();
    virtual     BOOL OnOpenDocument( const TCHAR* pszPathName );
    virtual     BOOL OnSaveDocument( const TCHAR* pszPathName );
    virtual BOOL CanCloseFrame( CFrameWnd* pFrame );
    virtual BOOL SaveModified(); // return TRUE if ok to continue
    virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace=TRUE);
    virtual BOOL OnUpdateDocument ();
    virtual COleServerItem* OnGetEmbeddedItem();
    virtual COleServerItem* OnGetLinkedItem( LPCTSTR lpszItemName );

    #ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
    #endif

        // Must be public so it can be called from Escape handler
        virtual void OnDeactivateUI(BOOL bUndoable);

protected:
        DECLARE_INTERFACE_MAP()

        BEGIN_INTERFACE_PART(PBPersistStg, IPersistStorage)
                INIT_INTERFACE_PART(CPBDoc, PBPersistStg)
                STDMETHOD(GetClassID)(LPCLSID);
                STDMETHOD(IsDirty)();
                STDMETHOD(InitNew)(LPSTORAGE);
                STDMETHOD(Load)(LPSTORAGE);
                STDMETHOD(Save)(LPSTORAGE, BOOL);
                STDMETHOD(SaveCompleted)(LPSTORAGE);
                STDMETHOD(HandsOffStorage)();
        END_INTERFACE_PART(PBPersistStg)

    protected:

    virtual BOOL OnNewDocument();
        virtual void OnShowControlBars(CFrameWnd *pFrame, BOOL bShow);

    // Generated message map functions
    protected:

    //{{AFX_MSG(CPBDoc)
        afx_msg void OnFileSaveAs();
        afx_msg void OnFileSave();
        //}}AFX_MSG

    virtual void ReportSaveLoadException(LPCTSTR, CException*, BOOL, UINT);

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/
