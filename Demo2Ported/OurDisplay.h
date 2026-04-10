#pragma once
#ifndef __S_DISPLAY_H__
#define __S_DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

	// Initialise la fenêtre native et le contexte OpenGL
	void init_display(const unsigned int width, const unsigned int height, const char* name, int (*drawfunc)(void));

	// Détruit la fenêtre et libère la mémoire
	void shutdown_display(void);

	// Échange les buffers OpenGL (Affiche l'image calculée à l'écran)
	void swap_buffer(void);

	// Lance la boucle d'événements (inputs, messages Windows, appels au rendu)
	void draw_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __S_DISPLAY_H__ */