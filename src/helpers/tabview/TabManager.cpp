/*
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * Modified by:
 *		A. Mosca, amoscaster@gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TabManager.h"

#include <Alert.h>
#include <Application.h>
#include <AbstractLayoutItem.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <Catalog.h>
#include <GroupView.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "TabContainerView.h"
#include "TabView.h"
#include "Utils.h"
#include "CircleColorMenuItem.h"

#include <stdexcept>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tab Manager"


const static BString kEmptyString;


enum {
	MSG_SCROLL_TABS_LEFT	= 'stlt',
	MSG_SCROLL_TABS_RIGHT	= 'strt',
	MSG_OPEN_TAB_MENU		= 'otmn'
};

// #pragma mark - Helper classes

class TabButton : public BButton {
public:
	TabButton(BMessage* message)
		: BButton("", message)
	{
	}

	virtual BSize MinSize()
	{
		return BSize(12, 12);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 flags = be_control_look->Flags(this);
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER;
		be_control_look->DrawInactiveTab(this, bounds, updateRect, base,
			0, borders);
		if (IsEnabled()) {
			rgb_color button = tint_color(base, 1.07);
			be_control_look->DrawButtonBackground(this, bounds, updateRect,
				button, flags, 0);
		}

		bounds.left = (bounds.left + bounds.right) / 2 - 6;
		bounds.top = (bounds.top + bounds.bottom) / 2 - 6;
		bounds.right = bounds.left + 12;
		bounds.bottom = bounds.top + 12;
		DrawSymbol(bounds, updateRect, base);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
	}
};


class ScrollLeftTabButton : public TabButton {
public:
	ScrollLeftTabButton(BMessage* message)
		: TabButton(message)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_LEFT_ARROW, 0, tint);
	}
};


class ScrollRightTabButton : public TabButton {
public:
	ScrollRightTabButton(BMessage* message)
		: TabButton(message)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		frame.OffsetBy(1, 0);
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_RIGHT_ARROW, 0, tint);
	}
};

#if 0
class NewTabButton : public TabButton {
public:
	NewTabButton(BMessage* message)
		: TabButton(message)
	{
		SetToolTip("New tab (Cmd-T)");
	}

	virtual BSize MinSize()
	{
		return BSize(18, 12);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		SetHighColor(tint_color(base, B_DARKEN_4_TINT));
		float inset = 3;
		frame.InsetBy(2, 2);
		frame.top++;
		frame.left++;
		FillRoundRect(BRect(frame.left, frame.top + inset,
			frame.right, frame.bottom - inset), 1, 1);
		FillRoundRect(BRect(frame.left + inset, frame.top,
			frame.right - inset, frame.bottom), 1, 1);
	}
};
#endif

class TabMenuTabButton : public TabButton {
public:
	TabMenuTabButton(BMessage* message)
		: TabButton(message)
		, fCloseTime(0)
	{
	}

	virtual BSize MinSize()
	{
		return BSize(18, 12);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_DOWN_ARROW, 0, B_DARKEN_4_TINT);
	}

	virtual void MouseDown(BPoint point)
	{
		// Don't reopen the menu if it's already open or freshly closed.
		bigtime_t clickSpeed = 2000000;
		get_click_speed(&clickSpeed);
		bigtime_t clickTime = Window()->CurrentMessage()->FindInt64("when");
		if (!IsEnabled() || (Value() == B_CONTROL_ON)
			|| clickTime < fCloseTime + clickSpeed) {
			return;
		}

		// Invoke must be called before setting B_CONTROL_ON
		// for the button to stay "down"
		Invoke();
		SetValue(B_CONTROL_ON);
	}

	virtual void MouseUp(BPoint point)
	{
		// Do nothing
	}

	void MenuClosed()
	{
		fCloseTime = system_time();
		SetValue(B_CONTROL_OFF);
	}

private:
	bigtime_t fCloseTime;
};


// #pragma mark - WebTabView


class WebTabView : public TabView {
public:
	WebTabView(TabManagerController* controller);
	~WebTabView();

	virtual BSize MaxSize();

	virtual	void DrawBackground(BView* owner, BRect frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront);
	virtual void DrawContents(BView* owner, BRect frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront);

	virtual void MouseDown(BPoint where, uint32 buttons);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage);

	void SetIcon(const BBitmap* icon);
	void SetColor(const rgb_color& color);
	const rgb_color Color() const;

private:
	void _DrawCloseButton(BView* owner, BRect& frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront);
	BRect _CloseRectFrame(BRect frame) const;

private:
	BBitmap* fIcon;
	rgb_color fColor;
	TabManagerController* fController;
	BPopUpMenu* fPopUpMenu;
	bool fOverCloseRect;
	bool fClicked;
};

class TabContainerGroup : public BGroupView {
public:
	TabContainerGroup(TabContainerView* tabContainerView)
		:
		BGroupView(B_HORIZONTAL, 0.0),
		fTabContainerView(tabContainerView),
		fScrollLeftTabButton(NULL),
		fScrollRightTabButton(NULL),
		fTabMenuButton(NULL)
	{
	}

	virtual void AttachedToWindow()
	{
		if (fScrollLeftTabButton != NULL)
			fScrollLeftTabButton->SetTarget(this);
		if (fScrollRightTabButton != NULL)
			fScrollRightTabButton->SetTarget(this);
		if (fTabMenuButton != NULL)
			fTabMenuButton->SetTarget(this);
	}


	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_SCROLL_TABS_LEFT:
				fTabContainerView->SetFirstVisibleTabIndex(
					fTabContainerView->FirstVisibleTabIndex() - 1);
				break;
			case MSG_SCROLL_TABS_RIGHT:
				fTabContainerView->SetFirstVisibleTabIndex(
					fTabContainerView->FirstVisibleTabIndex() + 1);
				break;
			case MSG_OPEN_TAB_MENU:
			{
				BPopUpMenu* tabMenu = new BPopUpMenu("tab menu", true, false);
				int tabCount = fTabContainerView->GetLayout()->CountItems();
				for (int i = 0; i < tabCount; i++) {
					WebTabView* tab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(i));

					if (tab) {
						BMenuItem* item = new CircleColorMenuItem(tab->Label(), tab->Color(), new BMessage());
						tabMenu->AddItem(item);
						if (tab->IsFront())
							item->SetMarked(true);
					}
				}

				// Force layout to get the final menu size. InvalidateLayout()
				// did not seem to work here.
				tabMenu->AttachedToWindow();
				BRect buttonFrame = fTabMenuButton->Frame();
				BRect menuFrame = tabMenu->Frame();
				BPoint openPoint = ConvertToScreen(buttonFrame.LeftBottom());
				// Open with the right side of the menu aligned with the right
				// side of the button and a little below.
				openPoint.x -= menuFrame.Width() - buttonFrame.Width();
				openPoint.y += 2;

				BMenuItem *selected = tabMenu->Go(openPoint, false, false,
					ConvertToScreen(buttonFrame));
				if (selected) {
					selected->SetMarked(true);
					int32 index = tabMenu->IndexOf(selected);
					if (index != B_ERROR)
						fTabContainerView->SelectTab(index);
				}
				fTabMenuButton->MenuClosed();
				delete tabMenu;

				break;
			}
			default:
				BGroupView::MessageReceived(message);
				break;
		}
	}

	void AddScrollLeftButton(TabButton* button)
	{
		fScrollLeftTabButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void AddScrollRightButton(TabButton* button)
	{
		fScrollRightTabButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void AddTabMenuButton(TabMenuTabButton* button)
	{
		fTabMenuButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void EnableScrollButtons(bool canScrollLeft, bool canScrollRight)
	{
		fScrollLeftTabButton->SetEnabled(canScrollLeft);
		fScrollRightTabButton->SetEnabled(canScrollRight);
		if (!canScrollLeft && !canScrollRight) {
			// hide scroll buttons
		} else {
			// show scroll buttons
		}
	}

private:
	TabContainerView*	fTabContainerView;
	TabButton*			fScrollLeftTabButton;
	TabButton*			fScrollRightTabButton;
	TabMenuTabButton*	fTabMenuButton;
};


class TabButtonContainer : public BGroupView {
public:
	TabButtonContainer()
		:
		BGroupView(B_HORIZONTAL, 0.0)
	{
		SetFlags(Flags() | B_WILL_DRAW);
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
		GroupLayout()->SetInsets(0, 6, 0, 0);
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		rgb_color base = LowColor();
		be_control_look->DrawInactiveTab(this, bounds, updateRect,
			base, 0, BControlLook::B_TOP_BORDER);
	}
};


class TabManagerController : public TabContainerView::Controller {
public:
	TabManagerController(TabManager* manager);

	virtual ~TabManagerController();

	virtual void TabSelected(int32 index, BMessage* selInfo)
	{
		fManager->DisplayTab(index);
		fManager->TabSelected(index, selInfo);
	}

	virtual bool HasFrames()
	{
		return false;
	}

	virtual TabView* CreateTabView();
#if 0
	virtual void DoubleClickOutsideTabs();
#endif
	virtual void UpdateTabScrollability(bool canScrollLeft,
		bool canScrollRight)
	{
		fTabContainerGroup->EnableScrollButtons(canScrollLeft, canScrollRight);
	}

	virtual	void SetToolTip(int32 selected)
	{
		BString toolTipText = fManager->GetToolTipText(selected);
		if (fCurrentToolTip == toolTipText)
			return;
		fCurrentToolTip = toolTipText;
		fManager->GetTabContainerView()->HideToolTip();
		fManager->GetTabContainerView()->SetToolTip(fCurrentToolTip.String());
	}

	void CloseTab(int32 index);

	void SetCloseButtonsAvailable(bool available)
	{
		fCloseButtonsAvailable = available;
	}

	bool CloseButtonsAvailable() const
	{
		return fCloseButtonsAvailable;
	}
#if 0
	void SetDoubleClickOutsideTabsMessage(const BMessage& message,
		const BMessenger& target);
#endif
	void SetTabContainerGroup(TabContainerGroup* tabContainerGroup)
	{
		fTabContainerGroup = tabContainerGroup;
	}

	void MoveTabs(int32 fromIndex, int32 toIndex)
	{
		fManager->MoveTabs(fromIndex, toIndex);
	}

	virtual void HandleTabMenuAction(BMessage* message)
	{
		switch (message->what) {
			case MSG_CLOSE_TAB:
			{
				int32 index = -1;
				if (message->FindInt32("tab_index", &index) == B_OK)
					fManager->CloseTabs(&index, 1);
				break;
			}
			case MSG_CLOSE_TABS_ALL: {
				int32 count = fManager->CountTabs();
				int32 tabsToClose[count];
				int32 added = 0;
				for (auto i = count - 1; i >= 0; i--) {
					tabsToClose[added++] = i;
				}
				fManager->CloseTabs(tabsToClose, added);
				break;
			}
			case MSG_CLOSE_TABS_OTHER: {
				int32 index = -1;
				int32 count = fManager->CountTabs();
				int32 tabsToClose[count];
				int32 added = 0;
				if (message->FindInt32("tab_index", &index) == B_OK) {
					for (auto i = count - 1; i >= 0; i--) {
						if (i != index)
							tabsToClose[added++] = i;
					}
					fManager->CloseTabs(tabsToClose, added);
				}
				break;
			}
			default:
				break;
		}
	}
private:
	TabManager*			fManager;
	TabContainerGroup*	fTabContainerGroup;
	bool				fCloseButtonsAvailable;
#if 0
	BMessage*			fDoubleClickOutsideTabsMessage;
	BMessenger			fTarget;
#endif
	BString				fCurrentToolTip;
};




WebTabView::WebTabView(TabManagerController* controller)
	:
	TabView(),
	fIcon(NULL),
	fColor(ui_color(B_PANEL_BACKGROUND_COLOR)),
	fController(controller),
	fPopUpMenu(nullptr),
	fOverCloseRect(false),
	fClicked(false)
{
	fPopUpMenu = new BPopUpMenu("tabmenu", false, false, B_ITEMS_IN_COLUMN);

	BMessage* closeMessage = new BMessage(MSG_CLOSE_TAB);
	closeMessage->AddPointer("tab_source", this);
	BMenuItem* close = new BMenuItem("Close", closeMessage);

	BMessage* closeAllMessage = new BMessage(MSG_CLOSE_TABS_ALL);
	closeAllMessage->AddPointer("tab_source", this);
	BMenuItem* closeAll = new BMenuItem("Close all", closeAllMessage);

	BMessage* closeOtherMessage = new BMessage(MSG_CLOSE_TABS_OTHER);
	closeOtherMessage->AddPointer("tab_source", this);
	BMenuItem* closeOther = new BMenuItem("Close other", closeOtherMessage);

	fPopUpMenu->AddItem(close);
	fPopUpMenu->AddItem(closeAll);
	fPopUpMenu->AddItem(closeOther);
}


WebTabView::~WebTabView()
{
	delete fIcon;
	delete fPopUpMenu;
}


static const int kIconSize = 18;
static const int kIconInset = 3;


BSize
WebTabView::MaxSize()
{
	// Account for icon.
	BSize size(TabView::MaxSize());
	size.height = max_c(size.height, kIconSize + kIconInset * 2);
	if (fIcon)
		size.width += kIconSize + kIconInset * 2;
	// Account for close button.
	size.width += size.height;
	return size;
}


/* virtual */
void
WebTabView::DrawBackground(BView* owner, BRect frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront)
{
	TabView::DrawBackground(owner, frame, updateRect, isFirst, isLast, isFront);
}


void
WebTabView::DrawContents(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	if (fController->CloseButtonsAvailable())
		_DrawCloseButton(owner, frame, updateRect, isFirst, isLast, isFront);

	if (fIcon) {
		BRect iconBounds(0, 0, kIconSize - 1, kIconSize - 1);
		// clip to icon bounds, if they are smaller
		if (iconBounds.Contains(fIcon->Bounds()))
			iconBounds = fIcon->Bounds();
		else {
			// Try to scale down the icon by an even factor so the
			// final size is between 14 and 18 pixel size. If this fails,
			// the icon will simply be displayed at 18x18.
			float scale = 2;
			while ((fIcon->Bounds().Width() + 1) / scale > kIconSize)
				scale *= 2;
			if ((fIcon->Bounds().Width() + 1) / scale >= kIconSize - 4
				&& (fIcon->Bounds().Height() + 1) / scale >= kIconSize - 4
				&& (fIcon->Bounds().Height() + 1) / scale <= kIconSize) {
				iconBounds.right = (fIcon->Bounds().Width() + 1) / scale - 1;
				iconBounds.bottom = (fIcon->Bounds().Height() + 1) / scale - 1;
			}
		}
		// account for borders
		frame.top -= 2.0f;
		BPoint iconPos(frame.left + kIconInset - 1,
			frame.top + floorf((frame.Height() - iconBounds.Height()) / 2));
		iconBounds.OffsetTo(iconPos);
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->DrawBitmap(fIcon, fIcon->Bounds(), iconBounds,
			B_FILTER_BITMAP_BILINEAR);
		owner->SetDrawingMode(B_OP_COPY);
		frame.left = frame.left + kIconSize + kIconInset * 2;
	}

	// Draw colored circle before text
	BRect circleFrame(frame);
	circleFrame.OffsetBy(0, 1);
	circleFrame.right = circleFrame.left + circleFrame.Height();
	circleFrame.InsetBy(5, 5);
	owner->SetHighColor(fColor);
	owner->FillEllipse(circleFrame);
	owner->SetHighColor(tint_color(fColor, B_DARKEN_1_TINT));
	owner->StrokeEllipse(circleFrame);
	frame.left = circleFrame.right + be_control_look->DefaultLabelSpacing();

	TabView::DrawContents(owner, frame, updateRect, isFirst, isLast, isFront);
}


void
WebTabView::MouseDown(BPoint where, uint32 buttons)
{
	if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		// Immediately close tab
		fController->CloseTab(ContainerView()->IndexOf(this));
		return;
	}

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		ContainerView()->ConvertToScreen(&where);
		BMessenger messenger(ContainerView());
		fPopUpMenu->SetTargetForItems(messenger);
		fPopUpMenu->Go(where, true);
		return;
	}

	BRect closeRect = _CloseRectFrame(Frame());
	if (!fController->CloseButtonsAvailable() || !closeRect.Contains(where)) {
		TabView::MouseDown(where, buttons);
		return;
	}

	fClicked = true;
	ContainerView()->Invalidate(closeRect);
}


void
WebTabView::MouseUp(BPoint where)
{
	if (!fClicked) {
		TabView::MouseUp(where);
		return;
	}

	fClicked = false;

	if (_CloseRectFrame(Frame()).Contains(where))
		fController->CloseTab(ContainerView()->IndexOf(this));
}


void
WebTabView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BRect closeRect = _CloseRectFrame(Frame());
	bool overCloseRect = closeRect.Contains(where);

	if (overCloseRect != fOverCloseRect
		&& fController->CloseButtonsAvailable()) {
		fOverCloseRect = overCloseRect;
		ContainerView()->Invalidate(closeRect);
	}

	TabView::MouseMoved(where, transit, dragMessage);
}


void
WebTabView::SetIcon(const BBitmap* icon)
{
	delete fIcon;
	if (icon)
		fIcon = new BBitmap(icon);
	else
		fIcon = NULL;
	LayoutItem()->InvalidateLayout();
}


void
WebTabView::SetColor(const rgb_color& color)
{
	fColor = color;
	if (ContainerView() != nullptr) {
		ContainerView()->Invalidate();
	}
}


const rgb_color
WebTabView::Color() const
{
	return fColor;
}


BRect
WebTabView::_CloseRectFrame(BRect frame) const
{
	frame.left = frame.right - frame.Height();
	return frame;
}

const int32 kBrightnessBreakValue = 126;
/*
static void inline
DecreaseContrastBy(float& tint, const float& value, const int& brightness)
{
	tint *= 1 + ((brightness >= kBrightnessBreakValue) ? -1 : +1) * value;
}
*/

static void inline
IncreaseContrastBy(float& tint, const float& value, const int& brightness)
{
	tint *= 1 + ((brightness >= kBrightnessBreakValue) ? +1 : -1) * value;
}


void WebTabView::_DrawCloseButton(BView* owner, BRect& frame,
	const BRect& updateRect, bool isFirst, bool isLast, bool isFront)
{
	BRect closeRect = _CloseRectFrame(frame);
	frame.right = closeRect.left - be_control_look->DefaultLabelSpacing();

	closeRect.InsetBy(closeRect.Width() * 0.30f, closeRect.Height() * 0.30f);

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	float tint = B_LIGHTEN_1_TINT;
	if (base.Brightness() >= kBrightnessBreakValue) {
		tint = B_DARKEN_1_TINT *1.2;
	}

	if (fOverCloseRect) {
		// Draw the button frame
		BRect buttonRect(closeRect.InsetByCopy(-4, -4));
		be_control_look->DrawButtonFrame(owner, buttonRect, updateRect,
			base, base,
			BControlLook::B_ACTIVATED | BControlLook::B_BLEND_FRAME);
		rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
		be_control_look->DrawButtonBackground(owner, buttonRect, updateRect,
			background, BControlLook::B_ACTIVATED);

	}

	// Draw the ×
	if (fClicked)
		IncreaseContrastBy(tint, .2, base.Brightness());
	base = tint_color(base, tint);
	owner->SetHighColor(base);
	owner->SetPenSize(2);
	closeRect.left +=1.0f;
	closeRect.top +=1.0f;
	owner->StrokeLine(closeRect.LeftTop(), closeRect.RightBottom());
	owner->StrokeLine(closeRect.LeftBottom(), closeRect.RightTop());
	owner->SetPenSize(1);
}


// #pragma mark - TabManagerController


TabManagerController::TabManagerController(TabManager* manager)
	:
	fManager(manager),
	fTabContainerGroup(NULL),
#if 0
	fCloseButtonsAvailable(false),
	fDoubleClickOutsideTabsMessage(NULL)
#endif
	fCloseButtonsAvailable(true)
{
}


TabManagerController::~TabManagerController()
{
#if 0
	delete fDoubleClickOutsideTabsMessage;
#endif
}


TabView*
TabManagerController::CreateTabView()
{
	return new WebTabView(this);
}

#if 0
void
TabManagerController::DoubleClickOutsideTabs()
{
	fTarget.SendMessage(fDoubleClickOutsideTabsMessage);
}
#endif

void
TabManagerController::CloseTab(int32 index)
{
	fManager->CloseTabs(&index, 1);
}

#if 0
void
TabManagerController::SetDoubleClickOutsideTabsMessage(const BMessage& message,
	const BMessenger& target)
{
	delete fDoubleClickOutsideTabsMessage;
	fDoubleClickOutsideTabsMessage = new BMessage(message);
	fTarget = target;
}
#endif

// #pragma mark - TabManager


TabManager::TabManager(const BMessenger& target)
    :
    fController(new TabManagerController(this)),
    fTarget(target)
{
#if 0
	fController->SetDoubleClickOutsideTabsMessage(*newTabMessage,
		be_app_messenger);
#endif
	fContainerView = new BView("web view container", 0);
	fCardLayout = new BCardLayout();
	fContainerView->SetLayout(fCardLayout);

	fTabContainerView = new TabContainerView(fController);
	fTabContainerGroup = new TabContainerGroup(fTabContainerView);

	fTabContainerGroup->GroupLayout()->SetInsets(0.0f, 0.0f, 0.0f, 0.0f);

	fController->SetTabContainerGroup(fTabContainerGroup);

#if INTEGRATE_MENU_INTO_TAB_BAR
	fMenu = new BMenu("Menu");
	BMenuBar* menuBar = new BMenuBar("Menu bar");
	menuBar->AddItem(fMenu);
	TabButtonContainer* menuBarContainer = new TabButtonContainer();
	menuBarContainer->GroupLayout()->AddView(menuBar);
	fTabContainerGroup->GroupLayout()->AddView(menuBarContainer, 0.0f);
#endif

	fTabContainerGroup->GroupLayout()->AddView(fTabContainerView);
	fTabContainerGroup->AddScrollLeftButton(new ScrollLeftTabButton(
		new BMessage(MSG_SCROLL_TABS_LEFT)));
	fTabContainerGroup->AddScrollRightButton(new ScrollRightTabButton(
		new BMessage(MSG_SCROLL_TABS_RIGHT)));
#if 0
	NewTabButton* newTabButton = new NewTabButton(newTabMessage);
	newTabButton->SetTarget(be_app);
	fTabContainerGroup->GroupLayout()->AddView(newTabButton, 0.0f);
#endif
	fTabContainerGroup->AddTabMenuButton(new TabMenuTabButton(
		new BMessage(MSG_OPEN_TAB_MENU)));

}


TabManager::~TabManager()
{
	delete fController;
}


void
TabManager::SetTarget(const BMessenger& target)
{
    fTarget = target;
}


const BMessenger&
TabManager::Target() const
{
    return fTarget;
}


#if INTEGRATE_MENU_INTO_TAB_BAR
BMenu*
TabManager::Menu() const
{
	return fMenu;
}
#endif


BView*
TabManager::TabGroup() const
{
	return fTabContainerGroup;
}


BView*
TabManager::GetTabContainerView() const
{
	return fTabContainerView;
}


BView*
TabManager::ContainerView() const
{
	return fContainerView;
}


BView*
TabManager::ViewForTab(int32 tabIndex) const
{
	BLayoutItem* item = fCardLayout->ItemAt(tabIndex);
	if (item != NULL)
		return item->View();
	return NULL;
}


int32
TabManager::TabForView(const BView* containedView) const
{
	int32 count = fCardLayout->CountItems();
	for (int32 i = 0; i < count; i++) {
		BLayoutItem* item = fCardLayout->ItemAt(i);
		if (item->View() == containedView)
			return i;
	}
	return -1;
}


bool
TabManager::HasView(const BView* containedView) const
{
	return TabForView(containedView) >= 0;
}

// Temporary hack to fix scintilla spoiling split
// scintilla + BTabView has the same behaviour
// BTextView + tabview is ok
extern BRect dirtyFrameHack;
#define DIRTY_HACK

/*
 * fTabContainerView-> SelectTab(tabIndex) calls
 * fController->TabSelected(index) who calls myself
 * fManager->SelectTab(index) so set sendMessage to true not to send message twice
 * but allowing caret position
 */


void
TabManager::TabSelected(int32 index, BMessage* selInfo)
{
	BMessage message;
	if (selInfo) {
		message = *selInfo;
	}
	message.what = TABMANAGER_TAB_SELECTED;
	message.AddInt32("index", index);
	fTarget.SendMessage(&message);
}

void
TabManager::SelectTab(int32 tabIndex, BMessage* selInfo)
{
	fTabContainerView->SelectTab(tabIndex, selInfo);
}

void
TabManager::DisplayTab(int32 tabIndex)
{
#if defined DIRTY_HACK
	fCardLayout->SetFrame(dirtyFrameHack);
#endif

	fCardLayout->SetVisibleItem(tabIndex);
}

void
TabManager::SelectTab(const BView* containedView)
{
	int32 tabIndex = TabForView(containedView);
	if (tabIndex >= 0)
		SelectTab(tabIndex);
}


int32
TabManager::SelectedTabIndex() const
{
//	return fCardLayout->VisibleIndex();

	int32 index = fCardLayout->VisibleIndex();

//	if (index < 0)
//		throw std::out_of_range("Negative index");
	if (index >= fCardLayout->CountItems())
		throw std::out_of_range("Overflown index");

	return index;
}


void
TabManager::CloseTabs(int32 tabIndex[], int32 size)
{
    BMessage message(TABMANAGER_TAB_CLOSE_MULTI);
	for (int32 i=0;i<size;i++)
		message.AddInt32("index", tabIndex[i]);
    fTarget.SendMessage(&message);
}


void
TabManager::AddTab(BView* view, const char* label, int32 index, BMessage* addInfo)
{
	fTabContainerView->AddTab(label, index);
#if defined DIRTY_HACK
	fCardLayout->SetFrame(dirtyFrameHack);
#endif
	fCardLayout->AddView(index, view);

	// Assuming nothing went wrong ...
	BMessage message;
	if (addInfo)
		message = *addInfo;
	message.what = TABMANAGER_TAB_NEW_OPENED;
	message.AddInt32("index", index);

	fTarget.SendMessage(&message);
}

void
TabManager::MoveTabs(int32 from, int32 to)
{
	WebTabView* oldTab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(from));
	const rgb_color color = oldTab != nullptr ? oldTab->Color() : ui_color(B_PANEL_BACKGROUND_COLOR);
	BString fromLabel = TabLabel(from);
	BView* view = RemoveTab(from);

	fTabContainerView->AddTab(fromLabel.String(), to);
#if defined DIRTY_HACK
	fCardLayout->SetFrame(dirtyFrameHack);
#endif
	fCardLayout->AddView(to, view);

	WebTabView* newTab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(to));
	if (newTab != nullptr)
		newTab->SetColor(color);

	SelectTab(to);
}


BView*
TabManager::RemoveTab(int32 index)
{
	// It's important to remove the view first, since
	// removing the tab will preliminary mess with the selected tab
	// and then item count of card layout and tab container will not
	// match yet.
	BLayoutItem* item = fCardLayout->RemoveItem(index);
	if (item == NULL)
		return NULL;

	bool isLast = index == CountTabs();

	TabView* tab = fTabContainerView->RemoveTab(index, SelectedTabIndex(), isLast);
	delete tab;

	BView* view = item->View();
	delete item;

	return view;
}


int32
TabManager::CountTabs() const
{
	return fCardLayout->CountItems();
}


void
TabManager::SetTabLabel(int32 tabIndex, const char* label)
{
	fTabContainerView->SetTabLabel(tabIndex, label);
}

const BString&
TabManager::TabLabel(int32 tabIndex)
{
	TabView* tab = fTabContainerView->TabAt(tabIndex);
	if (tab)
		return tab->Label();
	else
		return kEmptyString;
}

void
TabManager::SetTabIcon(const BView* containedView, const BBitmap* icon)
{
	WebTabView* tab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(
		TabForView(containedView)));
	if (tab)
		tab->SetIcon(icon);
}


void
TabManager::SetTabColor(const BView* containedView, const rgb_color& color)
{
	WebTabView* tab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(
		TabForView(containedView)));
	if (tab)
		tab->SetColor(color);
}


void
TabManager::SetCloseButtonsAvailable(bool available)
{
	if (available == fController->CloseButtonsAvailable())
		return;
	fController->SetCloseButtonsAvailable(available);
	fTabContainerView->Invalidate();
}
