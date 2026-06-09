#include "NetworkReplicant.h"
#include "Messages.h"

#include <Dragger.h>
#include <Font.h>
#include <String.h>
#include <Window.h>

#include <cstdio>

namespace lanterna {

static const char* kReplicantSig = "application/x-vnd.atomozero-LANterna";
static const rgb_color kBg = { 50, 50, 50, 220 };
static const rgb_color kFg = { 200, 230, 255, 255 };
static const rgb_color kFgZero = { 180, 180, 180, 255 };

NetworkReplicant::NetworkReplicant(BRect frame)
    : BView(frame, "LANterna Replicant", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      fCount(0)
{
    _UpdateLabel();
    // Dragger per rendere il view trascinabile sul Desktop.
    BRect draggerRect(Bounds());
    draggerRect.left = draggerRect.right - 7;
    draggerRect.top = draggerRect.bottom - 7;
    BDragger* dragger = new BDragger(draggerRect, this,
                                      B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
    AddChild(dragger);
}

NetworkReplicant::NetworkReplicant(BMessage* archive)
    : BView(archive),
      fCount(0)
{
    if (archive->FindInt32("device_count", &fCount) != B_OK)
        fCount = 0;
    _UpdateLabel();
}

BArchivable*
NetworkReplicant::Instantiate(BMessage* archive)
{
    if (!validate_instantiation(archive, "lanterna::NetworkReplicant"))
        return nullptr;
    return new NetworkReplicant(archive);
}

status_t
NetworkReplicant::Archive(BMessage* archive, bool deep) const
{
    status_t err = BView::Archive(archive, deep);
    if (err != B_OK) return err;

    archive->AddString("add_on", kReplicantSig);
    archive->AddString("class", "lanterna::NetworkReplicant");
    archive->AddInt32("device_count", fCount);
    return B_OK;
}

void
NetworkReplicant::AttachedToWindow()
{
    BView::AttachedToWindow();
    if (Parent())
        SetViewColor(B_TRANSPARENT_COLOR);
}

void
NetworkReplicant::Draw(BRect updateRect)
{
    BRect bounds = Bounds();

    // Sfondo arrotondato semitrasparente.
    SetHighColor(kBg);
    FillRoundRect(bounds, 4, 4);

    // Testo.
    SetHighColor(fCount > 0 ? kFg : kFgZero);
    SetFont(be_bold_font);

    font_height fh;
    GetFontHeight(&fh);
    float baseline = bounds.top + (bounds.Height() + fh.ascent - fh.descent) / 2.0f;
    float textWidth = StringWidth(fLabel.String());
    float x = bounds.left + (bounds.Width() - textWidth) / 2.0f;

    MovePenTo(x, baseline);
    DrawString(fLabel.String());
}

void
NetworkReplicant::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case kMsgReplicantUpdate:
        {
            int32 count = 0;
            if (message->FindInt32("count", &count) == B_OK)
                SetDeviceCount(count);
            break;
        }
        default:
            BView::MessageReceived(message);
            break;
    }
}

void
NetworkReplicant::SetDeviceCount(int32 count)
{
    fCount = count;
    _UpdateLabel();
    Invalidate();
}

void
NetworkReplicant::_UpdateLabel()
{
    fLabel.SetToFormat("LANterna: %d", static_cast<int>(fCount));
}

} // namespace lanterna
