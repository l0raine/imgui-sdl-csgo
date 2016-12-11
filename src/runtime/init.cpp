#include <dlfcn.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

// Pointer to 'SDL_GL_SwapWindow' in the jump table.
uintptr_t* swapwindow_ptr;

// Address of the original 'SDL_GL_SwapWindow'.
uintptr_t swapwindow_original;

// Helper function to resolve RIP relative addresses. (64-bit)
template <typename T> inline T* GetAbsoluteAddress(uintptr_t instruction_ptr, int offset, int size) {
	return reinterpret_cast<T*>(instruction_ptr + *reinterpret_cast<uint32_t*>(instruction_ptr + offset) + size);
};

// Create our replacement function.
void hkSwapWindow(SDL_Window* window) {
	// Get the original 'SDL_GL_SwapWindow' symbol from 'libSDL2-2.0.so.0'.
	static void (*oSDL_GL_SwapWindow) (SDL_Window*) = reinterpret_cast<void(*)(SDL_Window*)>(swapwindow_original);
	
	// Store OpenGL contexts.
	static SDL_GLContext original_context = SDL_GL_GetCurrentContext();
	static SDL_GLContext user_context = NULL;
	
	// Perform first-time initialization.
	if (!user_context) {
		// Create a new context for our rendering.
		user_context = SDL_GL_CreateContext(window);
		ImGui_ImplSdl_Init(window);
	}

	// Switch to our context.
	SDL_GL_MakeCurrent(window, user_context);

	// Perform UI rendering.
	ImGui_ImplSdl_NewFrame(window);

	ImGui::Text("Hello, world!");
	ImGui::Render();

	// Swap back to the game context.
	SDL_GL_MakeCurrent(window, original_context);

	// Call the original function.
	oSDL_GL_SwapWindow(window);
}

void __attribute__((constructor)) attach() {
	// Get the symbol address of 'SDL_GL_SwapWindow'.
	uintptr_t swapwindow_fn = reinterpret_cast<uintptr_t>(dlsym(RTLD_NEXT, "SDL_GL_SwapWindow"));

	// Get the address of 'SDL_GL_SwapWindow' in the jump table.
	swapwindow_ptr = GetAbsoluteAddress<uintptr_t>(swapwindow_fn, 3, 7);
	
	// Backup the original address.
	swapwindow_original = *swapwindow_ptr;

	// Write the address to our replacement function.
	*swapwindow_ptr = reinterpret_cast<uintptr_t>(&hkSwapWindow);
}

void __attribute__((destructor)) detach() {
	// Restore the original address.
	*swapwindow_ptr = swapwindow_original;
}