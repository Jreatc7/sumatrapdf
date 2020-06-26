/* Copyright 2020 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// define the following if you want shadows drawn around the pages
// #define DRAW_PAGE_SHADOWS

/* Describes many attributes of one page in one, convenient place */
struct PageInfo {
    /* data that is constant for a given page. page size in document units */
    RectD page{};

    /* data that is calculated when needed. actual content size within a page (View target) */
    RectD contentBox{};

    /* data that needs to be set before DisplayModel::Relayout().
       Determines whether a given page should be shown on the screen. */
    bool shown = false;

    /* data that changes when zoom and rotation changes */
    /* position and size within total area after applying zoom and rotation.
       Represents display rectangle for a given page.
       Calculated in DisplayModel::Relayout() */
    Rect pos{};

    /* data that changes due to scrolling. Calculated in DisplayModel::RecalcVisibleParts() */
    float visibleRatio; /* (0.0 = invisible, 1.0 = fully visible) */
    /* position of page relative to visible view port: pos.Offset(-viewPort.x, -viewPort.y) */
    Rect pageOnScreen{};

    // when zoomVirtual in DisplayMode is ZOOM_FIT_PAGE, ZOOM_FIT_WIDTH
    // or ZOOM_FIT_CONTENT, this is per-page zoom level
    float zoomReal;
};

/* The current scroll state (needed for saving/restoring the scroll position) */
/* coordinates are in user space units (per page) */
struct ScrollState {
    ScrollState() = default;
    explicit ScrollState(int page, double x, double y);
    bool operator==(const ScrollState& other) const;

    int page = 0;
    double x = 0;
    double y = 0;
};

struct DocumentTextCache;
struct TextSelection;
class TextSearch;
struct TextSel;
class Synchronizer;

// TODO: in hindsight, zoomVirtual is not a good name since it's either
// virtual zoom level OR physical zoom level. Would be good to find
// better naming scheme (call it zoomLevel?)

/* Information needed to drive the display of a given document on a screen.
   You can think of it as a model in the MVC pardigm.
   All the display changes should be done through changing this model via
   API and re-displaying things based on new display information */
struct DisplayModel : public Controller {
    DisplayModel(EngineBase* engine, ControllerCallback* cb);
    DisplayModel(DisplayModel const&) = delete;
    DisplayModel& operator=(DisplayModel const&) = delete;

    ~DisplayModel();

    // meta data
    const WCHAR* FilePath() const override;
    const WCHAR* DefaultFileExt() const override;
    int PageCount() const override;
    WCHAR* GetProperty(DocumentProperty prop) override;

    // page navigation (stateful)
    int CurrentPageNo() const override;
    void GoToPage(int pageNo, bool addNavPoint) override;
    bool CanNavigate(int dir) const override;
    void Navigate(int dir) override;

    // view settings
    void SetDisplayMode(DisplayMode mode, bool keepContinuous = false) override;
    DisplayMode GetDisplayMode() const override;
    void SetPresentationMode(bool enable) override;
    void SetZoomVirtual(float zoom, Point* fixPt) override;
    float GetZoomVirtual(bool absolute = false) const override;
    float GetNextZoomStep(float towards) const override;
    void SetViewPortSize(Size size) override;

    // table of contents
    TocTree* GetToc() override;
    void ScrollToLink(PageDestination* dest) override;
    PageDestination* GetNamedDest(const WCHAR* name) override;

    void GetDisplayState(DisplayState* ds) override;
    // asynchronously calls saveThumbnail (fails silently)
    void CreateThumbnail(Size size, const onBitmapRenderedCb& saveThumbnail) override;

    // page labels (optional)
    bool HasPageLabels() const override;
    WCHAR* GetPageLabel(int pageNo) const override;
    int GetPageByLabel(const WCHAR* label) const override;

    // common shortcuts
    bool ValidPageNo(int pageNo) const override;
    bool GoToNextPage() override;
    bool GoToPrevPage(bool toBottom = false) override;
    bool GoToFirstPage() override;
    bool GoToLastPage() override;

    // for quick type determination and type-safe casting
    DisplayModel* AsFixed() override;

    // the following is specific to DisplayModel

    EngineBase* GetEngine() const;
    Kind GetEngineType() const;

    // controller-specific data (easier to save here than on WindowInfo)
    Kind engineType = nullptr;
    // loaded from .smx file and added by the user
    Vec<Annotation*>* userAnnots = nullptr;

    Synchronizer* pdfSync = nullptr;

    DocumentTextCache* textCache = nullptr;
    TextSelection* textSelection = nullptr;
    // access only from Search thread
    TextSearch* textSearch = nullptr;

    PageInfo* GetPageInfo(int pageNo) const;

    /* current rotation selected by user */
    int GetRotation() const;
    float GetZoomReal(int pageNo) const;
    void Relayout(float zoomVirtual, int rotation);

    Rect GetViewPort() const;
    bool NeedHScroll() const;
    bool NeedVScroll() const;
    Size GetCanvasSize() const;

    bool PageShown(int pageNo) const;
    bool PageVisible(int pageNo) const;
    bool PageVisibleNearby(int pageNo) const;
    int FirstVisiblePageNo() const;
    bool FirstBookPageVisible() const;
    bool LastBookPageVisible() const;

    void ScrollXTo(int xOff);
    void ScrollXBy(int dx);
    void ScrollYTo(int yOff);
    void ScrollYBy(int dy, bool changePage);
    /* a "virtual" zoom level. Can be either a real zoom level in percent
       (i.e. 100.0 is original size) or one of virtual values ZOOM_FIT_PAGE,
       ZOOM_FIT_WIDTH or ZOOM_FIT_CONTENT, whose real value depends on draw area size */
    void RotateBy(int rotation);

    WCHAR* GetTextInRegion(int pageNo, RectD region);
    bool IsOverText(Point pt);
    IPageElement* GetElementAtPos(Point pt);

    int GetPageNoByPoint(Point pt);
    Point CvtToScreen(int pageNo, PointD pt);
    Rect CvtToScreen(int pageNo, RectD r);
    PointD CvtFromScreen(Point pt, int pageNo = INVALID_PAGE_NO);
    RectD CvtFromScreen(Rect r, int pageNo = INVALID_PAGE_NO);

    bool ShowResultRectToScreen(TextSel* res);

    ScrollState GetScrollState();
    void SetScrollState(ScrollState state);

    bool HasUnsavedAnnots();

    void CopyNavHistory(DisplayModel& orig);

    void SetInitialViewSettings(DisplayMode displayMode, int newStartPage, Size viewPort, int screenDPI);
    void SetDisplayR2L(bool r2l);
    bool GetDisplayR2L() const;

    bool ShouldCacheRendering(int pageNo);
    // called when we decide that the display needs to be redrawn
    void RepaintDisplay();

    /* allow resizing a window without triggering a new rendering (needed for window destruction) */
    bool dontRenderFlag = false;

    bool GetPresentationMode() const;

    void BuildPagesInfo();
    float ZoomRealFromVirtualForPage(float zoomVirtual, int pageNo) const;
    SizeD PageSizeAfterRotation(int pageNo, bool fitToContent = false) const;
    void ChangeStartPage(int startPage);
    Point GetContentStart(int pageNo);
    void RecalcVisibleParts();
    void RenderVisibleParts();
    void AddNavPoint();
    RectD GetContentBox(int pageNo);
    void CalcZoomReal(float zoomVirtual);
    void GoToPage(int pageNo, int scrollY, bool addNavPt = false, int scrollX = -1);
    bool GoToPrevPage(int scrollY);
    int GetPageNextToPoint(Point pt);

    EngineBase* engine = nullptr;

    /* an array of PageInfo, len of array is pageCount */
    PageInfo* pagesInfo = nullptr;

    DisplayMode displayMode = DM_AUTOMATIC;
    /* In non-continuous mode is the first page from a file that we're
       displaying.
       No meaning in continous mode. */
    int startPage = 1;

    /* size of virtual canvas containing all rendered pages. */
    Size canvasSize;
    /* size and position of the viewport on the canvas (resp size of the visible
       part of the canvase available for content (totalViewPortSize minus scroll bars)
       (canvasSize is always at least as big as viewPort.Size()) */
    Rect viewPort;
    /* total size of view port (draw area), including scroll bars */
    Size totalViewPortSize;

    WindowMargin windowMargin;
    Size pageSpacing;

    /* real zoom value calculated from zoomVirtual. Same as
       zoomVirtual * 0.01 * dpiFactor
       except for ZOOM_FIT_PAGE, ZOOM_FIT_WIDTH and ZOOM_FIT_CONTENT */
    float zoomReal = INVALID_ZOOM;
    float zoomVirtual = INVALID_ZOOM;
    int rotation = 0;
    /* dpi correction factor by which _zoomVirtual has to be multiplied in
       order to get _zoomReal */
    float dpiFactor = 1.0f;
    /* whether to display pages Left-to-Right or Right-to-Left.
       this value is extracted from the PDF document */
    bool displayR2L = false;

    /* when we're in presentation mode, _pres* contains the pre-presentation values */
    bool presentationMode = false;
    float presZoomVirtual = INVALID_ZOOM;
    DisplayMode presDisplayMode = DM_AUTOMATIC;

    Vec<ScrollState> navHistory;
    /* index of the "current" history entry (to be updated on navigation),
       resp. number of Back history entries */
    size_t navHistoryIx = 0;
};

int NormalizeRotation(int rotation);
