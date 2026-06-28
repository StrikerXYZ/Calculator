#include "core/intrinsic.c"
#include "core/random.c"
#include "core/memory.c"
#include "core/string.c"
#include "core/map_string.c"
#include "core/file.c"
#include "core/convert.c"
#include "core/console.c"
#include "core/math.c"
#include "core/widget.c"

#include "renderer/renderer_vulkan.c"
#include "platform/platform_windows.c"
#include "platform/platform_linux.c"

#if FEATURE_DEBUG
#include "test/math.c"
#endif

#include "application/glue.c"
#include "application/simulation.c"
#include "application/presentation.c"
#include "application/entry_point.c"

