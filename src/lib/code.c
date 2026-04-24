#include "yap/all.h"

void yap_emit_error_rangef(yap_ctx* ctx, yap_source* src, yap_code_range range, const char* fmt, ...){
	if (!ctx || !fmt) return;

	va_list ap;
	va_start(ap, fmt);
	char* msg = NULL;
	int fmt_res = vasprintf(&msg, fmt, ap);
	va_end(ap);

	if (fmt_res < 0 || !msg){
		msg = strus_copy("(failed to format error)");
	}

	yap_ctx_push_error(ctx, (yap_error){
		.kind = yap_error_pos,
		.src = src,
		.range = range,
		.msg = msg
	});
}
