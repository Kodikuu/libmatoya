#include "matoya.h"

// Your top level application context
struct context {
	MTY_App *app;
	bool quit;
};

// This function will fire once for each event
static void msg_func(const MTY_Msg *msg, void *opaque)
{
	struct context *ctx = opaque;

	if (msg->type == MTY_MSG_CLOSE)
		ctx->quit = true;
}

// This function fires once per "cycle", either blocked by a
// call to MTY_WindowPresent or limited by MTY_AppSetTimeout
static bool app_func(void *opaque)
{
	struct context *ctx = opaque;

	MTY_WindowPresent(ctx->app, 0, 1);

	return !ctx->quit;
}

int main(int argc, char **argv)
{
	// Set up the application object and attach it to your context
	struct context ctx = {0};
	ctx.app = MTY_AppCreate(app_func, msg_func, &ctx);
	if (!ctx.app)
		return 1;

	// Create a window
	MTY_WindowDesc desc = {
		.api = MTY_GFX_GL,
		.width = 800,
		.height = 600,
	};

	MTY_WindowCreate(ctx.app, "My Window", &desc);

	// Run the app -- blocks until your app_func returns false
	MTY_AppRun(ctx.app);
	MTY_AppDestroy(&ctx.app);

	return 0;
}
