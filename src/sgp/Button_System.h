// by Kris Morness (originally created by Bret Rowden)

#pragma once

#include "MouseSystem.h"

#include <string_theory/string>


#define MAX_BUTTON_PICS 256


// Some GUI_BUTTON system defines
#define BUTTON_NO_IMAGE    -1

//effects how the button is rendered.
#define BUTTON_TYPES (BUTTON_QUICK | BUTTON_GENERIC | BUTTON_HOT_SPOT | BUTTON_CHECKBOX)

//button flags
#define BUTTON_TOGGLE           0x00000000
#define BUTTON_QUICK            0x00000000
#define BUTTON_ENABLED          0x00000001
#define BUTTON_CLICKED_ON       0x00000002
#define BUTTON_GENERIC          0x00000020
#define BUTTON_HOT_SPOT         0x00000040
#define BUTTON_SELFDELETE_IMAGE 0x00000080
#define BUTTON_DELETION_PENDING 0x00000100
#define BUTTON_DIRTY            0x00000400
#define BUTTON_CHECKBOX         0x00001000
#define BUTTON_NEWTOGGLE        0x00002000
#define BUTTON_FORCE_UNDIRTY    0x00004000 // no matter what happens this buttons does NOT get marked dirty
#define BUTTON_NO_DUPLICATE     0x80000000 // Exclude button from duplicate check


extern SGPVSurface* ButtonDestBuffer;

struct GUI_BUTTON;

// GUI_BUTTON callback function type
typedef std::function<void(GUI_BUTTON*, UINT32)> GUI_CALLBACK;

GUI_CALLBACK ButtonCallbackPrimarySecondary(
	GUI_CALLBACK primaryAction,
	GUI_CALLBACK secondaryAction,
	GUI_CALLBACK allEvents = nullptr,
	bool triggerPrimaryOnMouseDown = false
);

// GUI_BUTTON structure definitions.
struct GUI_BUTTON
{
	GUI_BUTTON(UINT32 flags, INT16 left, INT16 top, INT16 width, INT16 height, INT8 priority, GUI_CALLBACK click, GUI_CALLBACK move);
	~GUI_BUTTON();

	bool Clicked() const { return uiFlags & BUTTON_CLICKED_ON; }

	bool Enabled() const { return uiFlags & BUTTON_ENABLED; }

	// Set the text that will be displayed as the FastHelp
	void SetFastHelpText(const ST::string& str);

	void Hide();
	void Show();

	// Draw the button on the screen.
	void Draw();

	void SpecifyDownTextColors(INT16 fore_colour_down, INT16 shadow_colour_down);
	void SpecifyHilitedTextColors(INT16 fore_colour_highlighted, INT16 shadow_colour_highlighted);

	enum Justification
	{
		TEXT_LEFT   = -1,
		TEXT_CENTER =  0,
		TEXT_RIGHT  =  1
	};
	void SpecifyTextJustification(Justification);

	void SpecifyText(const ST::string& str);
	void SpecifyGeneralTextAttributes(const ST::string& str, SGPFont font, INT16 fore_colour, INT16 shadow_colour);
	void SpecifyTextOffsets(INT8 text_x_offset, INT8 text_y_offset, BOOLEAN shift_text);
	void SpecifyTextSubOffsets(INT8 text_x_offset, INT8 text_y_offset, BOOLEAN shift_text);
	void SpecifyTextWrappedWidth(INT16 wrapped_width);

	void AllowDisabledFastHelp();

	enum DisabledStyle
	{
		DISABLED_STYLE_NONE,    // for dummy buttons, panels, etc.  Always displays normal state.
		DISABLED_STYLE_DEFAULT, // if button has text then shade, else hatch
		DISABLED_STYLE_HATCHED, // always hatches the disabled button
		DISABLED_STYLE_SHADED   // always shades the disabled button 25% darker
	};
	void SpecifyDisabledStyle(DisabledStyle);

	/* Note:  Text is always on top
	 * If fShiftImage is true, then the image will shift down one pixel and right
	 * one pixel just like the text does.
	 */
	void SpecifyIcon(SGPVObject const* icon, UINT16 usVideoObjectIndex, INT8 bXOffset, INT8 bYOffset, BOOLEAN fShiftImage);

	// will simply set the cursor for the mouse region the button occupies
	void SetCursor(UINT16 const cursor) { Area.ChangeCursor(cursor); }

	// Coordinates where button is on the screen
	INT16 X()            const { return Area.RegionTopLeftX; }
	INT16 Y()            const { return Area.RegionTopLeftY; }
	INT16 W()            const { return Area.RegionBottomRightX - Area.RegionTopLeftX; }
	INT16 H()            const { return Area.RegionBottomRightY - Area.RegionTopLeftY; }
	INT16 BottomRightX() const { return Area.RegionBottomRightX; }
	INT16 BottomRightY() const { return Area.RegionBottomRightY; }

	INT16 MouseX()    const { return Area.MouseXPos; }
	INT16 MouseY()    const { return Area.MouseYPos; }
	INT16 RelativeX() const { return Area.RelativeXPos; }
	INT16 RelativeY() const { return Area.RelativeYPos; }

	INT32 GetUserData() const { return User.Data; }
	void  SetUserData(INT32 const data) { User.Data = data; }

	template<typename T> T* GetUserPtr() const { return static_cast<T*>(User.Ptr); }
	void SetUserPtr(void* const p) { User.Ptr = p; }

	BUTTON_PICS* image;         // Image to use (see DOCs for details)
	MouseRegion  Area;          // Mouse System's mouse region to use for this button
	GUI_CALLBACK ClickCallback; // Button Callback when button is clicked
	GUI_CALLBACK MoveCallback;  // Button Callback when mouse moved on this region
	UINT32       uiFlags;       // Button state flags etc.( 32-bit )
	UINT32       uiOldFlags;    // Old flags from previous render loop
	union                       // Place holder for user data etc.
	{
		INT32 Data;
		void* Ptr;
	} User;

	// For buttons with text
	ST::utf32_buffer codepoints;
	SGPFont      usFont;              // font for text
	INT16        sForeColor;          // text colors if there is text
	INT16        sShadowColor;
	INT16        sForeColorDown;      // text colors when button is down (optional)
	INT16        sShadowColorDown;
	INT16        sForeColorHilited;   // text colors when button is down (optional)
	INT16        sShadowColorHilited;
	INT8         bJustification;      // BUTTON_TEXT_LEFT, BUTTON_TEXT_CENTER, BUTTON_TEXT_RIGHT
	INT8         bTextXOffset;
	INT8         bTextYOffset;
	INT8         bTextXSubOffSet;
	INT8         bTextYSubOffSet;
	BOOLEAN      fShiftText;
	INT16        sWrappedWidth;

	// For buttons with icons (don't confuse this with quickbuttons which have up to 5 states)
	const SGPVObject* icon;
	INT16        usIconIndex;
	INT8         bIconXOffset; // -1 means horizontally centered
	INT8         bIconYOffset; // -1 means vertically centered
	BOOLEAN      fShiftImage;  // if true, icon is shifted +1,+1 when button state is down.

	UINT8        ubToggleButtonActivated;

	INT8         bDisabledStyle; // Button disabled style
	UINT8        ubSoundSchemeID;
};

// For backwards compatibility only; please do not use this alias in new code.
using GUIButtonRef = GUI_BUTTON *;


/* Initializes the GUI button system for use. Must be called before using any
 * other button functions.
 */
void InitButtonSystem(void);

/* Shuts down and cleans up the GUI button system. Must be called before exiting
 * the program.  Button functions should not be used after calling this
 * function.
 */
void ShutdownButtonSystem(void);

// Loads an image file for use as a button icon.
INT16 LoadGenericButtonIcon(const char* filename);

// Removes a button icon graphic from the system
void UnloadGenericButtonIcon(INT16 GenImg);

// Load images for use with QuickButtons.
BUTTON_PICS* LoadButtonImage(const char* filename, INT32 Grayed, INT32 OffNormal, INT32 OffHilite, INT32 OnNormal, INT32 OnHilite);
BUTTON_PICS* LoadButtonImage(char const* filename, INT32 off_normal, INT32 on_normal);

/* Uses a previously loaded quick button image for use with QuickButtons.  The
 * function simply duplicates the vobj!
 */
BUTTON_PICS* UseLoadedButtonImage(BUTTON_PICS* LoadedImg, INT32 Grayed, INT32 OffNormal, INT32 OffHilite, INT32 OnNormal, INT32 OnHilite);
BUTTON_PICS* UseLoadedButtonImage(BUTTON_PICS* img, INT32 off_normal, INT32 on_normal);

// Removes a QuickButton image from the system.
void UnloadButtonImage(BUTTON_PICS*);

// Enables an already created button.
void EnableButton(GUI_BUTTON *);

/* Disables a button. The button remains in the system list, and can be
 * reactivated by calling EnableButton.  Diabled buttons will appear "grayed
 * out" on the screen (unless the graphics for such are not available).
 */
void DisableButton(GUI_BUTTON *);

void EnableButton(GUI_BUTTON *, bool enable);

/* Removes a button from the system's list. All memory associated with the
 * button is released. The pointer itself is set to null.
 */
void RemoveButton(GUI_BUTTON *&);

void HideButton(GUI_BUTTON *);
void ShowButton(GUI_BUTTON *);

void RenderButtons(void);

extern BOOLEAN gfRenderHilights;

/* Creates a QuickButton. QuickButtons only have graphics associated with them.
 * They cannot be re-sized, nor can the graphic be changed.  Providing you have
 * allocated your own image, this is a somewhat simplified function.
 */
GUI_BUTTON * QuickCreateButton(BUTTON_PICS* image, INT16 x, INT16 y, INT16 priority, GUI_CALLBACK click);
GUI_BUTTON * QuickCreateButtonNoMove(BUTTON_PICS* image, INT16 x, INT16 y, INT16 priority, GUI_CALLBACK click);
GUI_BUTTON * QuickCreateButtonToggle(BUTTON_PICS* image, INT16 x, INT16 y, INT16 priority, GUI_CALLBACK click);

GUI_BUTTON * QuickCreateButtonImg(const char* gfx, INT32 grayed, INT32 off_normal, INT32 off_hilite, INT32 on_normal, INT32 on_hilite, INT16 x, INT16 y, INT16 priority, GUI_CALLBACK click);
GUI_BUTTON * QuickCreateButtonImg(const char* gfx, INT32 off_normal, INT32 on_normal, INT16 x, INT16 y, INT16 priority, GUI_CALLBACK click);

GUI_BUTTON * CreateCheckBoxButton(INT16 x, INT16 y, const char* filename, INT16 Priority, GUI_CALLBACK ClickCallback);

// Creates an Iconic type button.
GUI_BUTTON * CreateIconButton(INT16 Icon, INT16 IconIndex, INT16 xloc, INT16 yloc, INT16 w, INT16 h, INT16 Priority, GUI_CALLBACK ClickCallback);

/* Creates a button like HotSpot. HotSpots have no graphics associated with
 * them.
 */
GUI_BUTTON * CreateHotSpot(INT16 xloc, INT16 yloc, INT16 Width, INT16 Height, INT16 Priority, GUI_CALLBACK ClickCallback);

// Creates a generic button with text on it.
GUI_BUTTON * CreateTextButton(const ST::string& str, SGPFont font, INT16 sForeColor, INT16 sShadowColor, INT16 xloc, INT16 yloc, INT16 w, INT16 h, INT16 Priority, GUI_CALLBACK ClickCallback);

GUI_BUTTON * CreateIconAndTextButton(BUTTON_PICS* Image, const ST::string& str, SGPFont font, INT16 sForeColor, INT16 sShadowColor, INT16 sForeColorDown, INT16 sShadowColorDown, INT16 xloc, INT16 yloc, INT16 Priority, GUI_CALLBACK ClickCallback);

/* This is technically not a clickable button, but just a label with text. It is
 * implemented as button */
GUI_BUTTON * CreateLabel(const ST::string& str, SGPFont font, INT16 forecolor, INT16 shadowcolor, INT16 x, INT16 y, INT16 w, INT16 h, INT16 priority);

void MarkAButtonDirty(GUI_BUTTON *); // will mark only selected button dirty
void MarkButtonsDirty(void);// Function to mark buttons dirty ( all will redraw at next RenderButtons )
void UnMarkButtonDirty(GUI_BUTTON *);  // unmark button
void UnmarkButtonsDirty(void); // unmark ALL the buttoms on the screen dirty
void ForceButtonUnDirty(GUI_BUTTON *); // forces button undirty no matter the reason, only lasts one frame


struct ButtonDimensions
{
	UINT32 w;
	UINT32 h;
};

const ButtonDimensions* GetDimensionsOfButtonPic(const BUTTON_PICS*);

UINT16 GetGenericButtonFillColor(void);

void ReleaseAnchorMode(void);
