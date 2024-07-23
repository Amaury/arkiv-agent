#define YANSI_IS_YANSI
#include "yansi.h"

/* ********** global variable ********** */
bool _yansi_enabled_gl = true;

/* ********** functions ********** */
/* Enable ANSI control characters. */
void yansi_enable(void) {
	_yansi_enabled_gl = true;
}
/* Disable ANSI control characters. */
void yansi_disable(void) {
	_yansi_enabled_gl = false;
}
/* Enable or disable ANSI control characters. */
void yansi_activate(bool enable) {
	_yansi_enabled_gl = enable;
}

