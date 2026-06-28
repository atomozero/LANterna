/*
 * Copyright 2026 atomozero. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LANTERNA_UI_HEADERVIEW_H
#define LANTERNA_UI_HEADERVIEW_H


#include <InterfaceDefs.h>
#include <Messenger.h>
#include <String.h>
#include <View.h>

class BBitmap;


namespace lanterna {

// Stato visivo del banner. Guida il colore del dot di stato sovrapposto al
// tile-logo. Mappato sulle fasi di scansione di LANterna:
//   kHeaderIdle      -> in attesa, nessuna scansione attiva  (grigio)
//   kHeaderProgress  -> scansione in corso                   (ambra)
//   kHeaderDone      -> scansione conclusa con successo      (verde)
//   kHeaderError     -> nessuna interfaccia / impossibile    (rosso)
enum HeaderState {
	kHeaderIdle,
	kHeaderProgress,
	kHeaderDone,
	kHeaderError
};


// Il banner scuro in cima alla finestra principale. Stessa identita di
// Sotoportego: sfondo slate, "tile logo" colorato a sinistra con il dot di
// stato sovrapposto, e a destra il nome dell'app + una riga di sottotitolo
// (es. "Pronto.", "Scansione 42%...", "Fatto. 17 device trovati."). Tutto
// disegnato a mano (senza child view) per mantenere il layout semplice e
// lasciare che siano i colori a raccontare lo stato a colpo d'occhio.
class HeaderView : public BView {
public:
								HeaderView(const char* name);
	virtual						~HeaderView();

			// Aggiorna lo stato (guida il colore del dot) e la riga di
			// metadata. Le chiamate sono idempotenti.
			void				SetState(HeaderState state);
			void				SetSubtitle(const char* text);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	// Rasterizza lo stesso tile-logo del banner in un BBitmap RGBA della
	// dimensione richiesta. Utile per una futura About dialog che mantenga
	// la stessa identita. Il chiamante e' proprietario del bitmap (puo'
	// tornare NULL in caso di errore).
	static	BBitmap*			MakeLogoBitmap(float size);

	// Messenger che riceve un BMessage(what) quando il tile viene
	// "tappato" abbastanza volte di fila -- innesco dell'easter egg.
	// No-op se il messenger non e' valido.
			void				SetEasterEggTarget(const BMessenger& target,
									uint32 what);

private:
			void				_DrawLogoTile(BRect rect);
			void				_DrawStatusDot(BRect iconRect);

			HeaderState			fState;
			BString				fSubtitle;
			BMessenger			fEasterTarget;
			uint32				fEasterWhat;
			bigtime_t			fLastTileClick;
			int32				fTileClickStreak;

	// Cache del HVIF rasterizzato. Riallocato solo se cambia la dimensione
	// richiesta: un redraw alla stessa size diventa un DrawBitmap piatto,
	// non un'altra rasterizzazione vettoriale + allocazione RGBA.
			BBitmap*			fCachedIcon;
			float				fCachedIconSize;
};


} // namespace lanterna

#endif	// LANTERNA_UI_HEADERVIEW_H
