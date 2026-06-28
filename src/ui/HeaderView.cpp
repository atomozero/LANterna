/*
 * Copyright 2026 atomozero. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "HeaderView.h"

#include <Bitmap.h>
#include <Font.h>
#include <IconUtils.h>

#include "beacon_icon_data.h"


namespace lanterna {

// Costanti visive. Stesse tonalita' slate/title/subtitle usate da Sotoportego
// cosi' le due app sembrano della stessa famiglia sul desktop.
static const rgb_color kHeaderBg		= { 40, 50, 65, 255 };
static const rgb_color kHeaderTitle		= { 245, 245, 245, 255 };
static const rgb_color kHeaderSubtitle	= { 180, 195, 210, 255 };
static const rgb_color kDotStroke		= { 255, 255, 255, 255 };

// Fill di fallback per il tile-logo se la rasterizzazione HVIF fallisce.
static const rgb_color kLogoFill		= { 90, 155, 213, 255 };

// Accenti di stato per il dot sovrapposto al tile.
static const rgb_color kAccentIdle		= { 160, 160, 160, 255 };
static const rgb_color kAccentProgress	= { 224, 160, 48, 255 };
static const rgb_color kAccentDone		= { 90, 200, 120, 255 };
static const rgb_color kAccentError		= { 220, 80, 80, 255 };

static const float kHeaderHeight		= 64.0f;
static const float kIconX				= 14.0f;
static const float kIconY				= 12.0f;
static const float kIconSize			= 40.0f;
static const float kTextX				= 68.0f;
static const float kTitleBaselineY		= 27.0f;
static const float kSubtitleBaselineY	= 47.0f;


static rgb_color
_AccentFor(HeaderState state)
{
	switch (state) {
		case kHeaderProgress:
			return kAccentProgress;
		case kHeaderDone:
			return kAccentDone;
		case kHeaderError:
			return kAccentError;
		case kHeaderIdle:
		default:
			return kAccentIdle;
	}
}


HeaderView::HeaderView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_SUPPORTS_LAYOUT | B_FULL_UPDATE_ON_RESIZE),
	fState(kHeaderIdle),
	fSubtitle(""),
	fEasterTarget(),
	fEasterWhat(0),
	fLastTileClick(0),
	fTileClickStreak(0),
	fCachedIcon(NULL),
	fCachedIconSize(0.0f)
{
	SetViewColor(kHeaderBg);
	SetLowColor(kHeaderBg);
}


HeaderView::~HeaderView()
{
	delete fCachedIcon;
}


void
HeaderView::SetEasterEggTarget(const BMessenger& target, uint32 what)
{
	fEasterTarget = target;
	fEasterWhat = what;
}


void
HeaderView::SetState(HeaderState state)
{
	if (state == fState)
		return;
	fState = state;
	Invalidate();
}


void
HeaderView::SetSubtitle(const char* text)
{
	BString next(text != NULL ? text : "");
	if (next == fSubtitle)
		return;
	fSubtitle = next;
	Invalidate();
}


void
HeaderView::Draw(BRect /*updateRect*/)
{
	BRect bounds = Bounds();

	SetHighColor(kHeaderBg);
	FillRect(bounds);

	BRect iconRect(kIconX, kIconY, kIconX + kIconSize - 1,
		kIconY + kIconSize - 1);
	_DrawLogoTile(iconRect);
	_DrawStatusDot(iconRect);

	// SetLowColor pari a kHeaderBg cosi' i glifi antialiasati si fondono
	// puliti contro lo slate.
	SetDrawingMode(B_OP_OVER);

	BFont titleFont(be_bold_font);
	titleFont.SetSize(18.0f);
	SetFont(&titleFont);
	SetHighColor(kHeaderTitle);
	DrawString("LANterna", BPoint(kTextX, kTitleBaselineY));

	BFont subFont(be_plain_font);
	subFont.SetSize(11.0f);
	SetFont(&subFont);
	SetHighColor(kHeaderSubtitle);
	DrawString(fSubtitle.String(), BPoint(kTextX, kSubtitleBaselineY));
}


// Rasterizza l'HVIF del brand in un BBitmap RGBA quadrato di `size` pixel.
// Ritorna NULL se la rasterizzazione fallisce. Il chiamante e' proprietario.
static BBitmap*
_RenderHvif(float size)
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0,
		B_RGBA32);
	if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return NULL;
	}
	status_t result = BIconUtils::GetVectorIcon(
		(const uint8*)kIconHvif, kIconHvifSize, bitmap);
	if (result != B_OK) {
		delete bitmap;
		return NULL;
	}
	return bitmap;
}


void
HeaderView::_DrawLogoTile(BRect rect)
{
	// Rasterizzare l'HVIF e' costoso (BIconUtils + allocazione RGBA). La
	// cache vale finche' la dimensione richiesta non cambia -- Draw() gira
	// ad ogni Invalidate, cioe' ad ogni cambio di stato/sottotitolo e ad
	// ogni resize.
	if (fCachedIcon == NULL || fCachedIconSize != rect.Width()) {
		delete fCachedIcon;
		fCachedIcon = _RenderHvif(rect.Width());
		fCachedIconSize = rect.Width();
	}

	if (fCachedIcon != NULL) {
		// Alpha-blend cosi' i pixel trasparenti dell'icona lasciano
		// vedere lo slate del banner.
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		DrawBitmap(fCachedIcon, rect.LeftTop());
		SetDrawingMode(B_OP_COPY);
		return;
	}

	// Fallback: tile arrotondato col colore brand se l'HVIF non ha
	// potuto rasterizzare per qualsiasi motivo.
	float radius = rect.Width() * 0.18f;
	SetHighColor(kLogoFill);
	FillRoundRect(rect, radius, radius);
}


void
HeaderView::MouseDown(BPoint where)
{
	// Easter egg: 7 tap sul tile entro 3 secondi mandano fEasterWhat.
	// Il counter scatta solo per click che colpiscono davvero il tile,
	// cosi' i click sulla parte di testo non interferiscono.
	BRect tile(kIconX, kIconY, kIconX + kIconSize - 1,
		kIconY + kIconSize - 1);
	if (!tile.Contains(where))
		return;
	if (!fEasterTarget.IsValid() || fEasterWhat == 0)
		return;

	const bigtime_t kStreakWindow = 3 * 1000000;	// 3 secondi
	const int32 kStreakGoal = 7;

	bigtime_t now = system_time();
	if (now - fLastTileClick > kStreakWindow)
		fTileClickStreak = 0;
	fLastTileClick = now;
	fTileClickStreak++;

	if (fTileClickStreak >= kStreakGoal) {
		fTileClickStreak = 0;
		BMessage trigger(fEasterWhat);
		fEasterTarget.SendMessage(&trigger);
	}
}


void
HeaderView::_DrawStatusDot(BRect iconRect)
{
	// Cerchietto pieno sovrapposto all'angolo basso-destra del tile,
	// bordato di bianco per restare nitido contro qualunque colore.
	const float dotSize = 14.0f;
	BRect dot(0, 0, dotSize - 1, dotSize - 1);
	dot.OffsetTo(iconRect.right - dotSize + 4, iconRect.bottom - dotSize + 4);

	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(_AccentFor(fState));
	FillEllipse(dot);
	SetHighColor(kDotStroke);
	StrokeEllipse(dot);
	SetDrawingMode(B_OP_COPY);
}


BSize
HeaderView::MinSize()
{
	return BSize(360.0f, kHeaderHeight);
}


BSize
HeaderView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, kHeaderHeight);
}


BSize
HeaderView::PreferredSize()
{
	return BSize(540.0f, kHeaderHeight);
}


BBitmap*
HeaderView::MakeLogoBitmap(float size)
{
	return _RenderHvif(size);
}


} // namespace lanterna
