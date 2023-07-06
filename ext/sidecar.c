#include "ddtrace.h"
#include "configuration.h"
#include "coms.h"
#include "logging.h"
#include <components-rs/ddtrace.h>
#include "sidecar.h"
#include "telemetry.h"

ZEND_EXTERN_MODULE_GLOBALS(ddtrace);

ddog_SidecarTransport *ddtrace_sidecar;
ddog_Endpoint *ddtrace_endpoint;
struct ddog_InstanceId *ddtrace_sidecar_instance_id;
static uint8_t dd_sidecar_formatted_session_id[36];

static void ddtrace_set_sidecar_globals(void) {
    uint8_t formatted_run_time_id[36];
    ddtrace_format_runtime_id(&formatted_run_time_id);
    ddog_CharSlice runtime_id = (ddog_CharSlice) {.ptr = (char *) formatted_run_time_id, .len = sizeof(formatted_run_time_id)};
    ddog_CharSlice session_id = (ddog_CharSlice) {.ptr = (char *) dd_sidecar_formatted_session_id, .len = sizeof(dd_sidecar_formatted_session_id)};
    ddtrace_sidecar_instance_id = ddog_sidecar_instanceId_build(session_id, runtime_id);
}

static bool dd_sidecar_connection_init(void) {
    if (!ddtrace_ffi_try("Failed connecting to the sidecar", ddog_sidecar_connect_php(&ddtrace_sidecar, get_global_DD_INSTRUMENTATION_TELEMETRY_ENABLED()))) {
        ddtrace_sidecar = NULL;
        return false;
    }

    if (get_global_DD_TRACE_BYPASS_AGENT() && ZSTR_LEN(get_global_DD_API_KEY())) {
        ddtrace_endpoint = ddog_endpoint_from_api_key(dd_zend_string_to_CharSlice(get_global_DD_API_KEY()));
    } else {
        char *agent_url = ddtrace_agent_url();
        ddtrace_endpoint = ddog_endpoint_from_url((ddog_CharSlice) {.ptr = agent_url, .len = strlen(agent_url)});
        free(agent_url);
    }
    if (!ddtrace_endpoint) {
        ddog_sidecar_transport_drop(ddtrace_sidecar);
        ddtrace_sidecar = NULL;
        return false;
    }

    if (!ddtrace_sidecar_instance_id) {
        ddtrace_format_runtime_id(&dd_sidecar_formatted_session_id);
        ddtrace_set_sidecar_globals();

        if (get_global_DD_INSTRUMENTATION_TELEMETRY_ENABLED()) {
            ddtrace_telemetry_first_init();
        }
    }

    ddog_CharSlice session_id = (ddog_CharSlice) {.ptr = (char *) dd_sidecar_formatted_session_id, .len = sizeof(dd_sidecar_formatted_session_id)};
    ddog_sidecar_session_set_config(&ddtrace_sidecar, session_id, ddtrace_endpoint, DDOG_CHARSLICE_C("php"), DDOG_CHARSLICE_C(PHP_DDTRACE_VERSION),
                                    get_global_DD_TRACE_AGENT_FLUSH_INTERVAL(),
                                    get_global_DD_TRACE_AGENT_STACK_INITIAL_SIZE(),
                                    get_global_DD_TRACE_AGENT_STACK_BACKLOG() * get_global_DD_TRACE_AGENT_MAX_PAYLOAD_SIZE());

    return true;
}

void ddtrace_sidecar_setup(void) {
    dd_sidecar_connection_init();
}

void ddtrace_sidecar_ensure_active(void) {
    if (!ddtrace_sidecar || ddog_sidecar_is_closed(&ddtrace_sidecar)) {
        if (ddtrace_sidecar) {
            ddog_sidecar_transport_drop(ddtrace_sidecar);
        }
        dd_sidecar_connection_init();
    }
}

void ddtrace_sidecar_shutdown(void) {
    if (ddtrace_sidecar_instance_id) {
        ddog_sidecar_instanceId_drop(ddtrace_sidecar_instance_id);
    }
    if (ddtrace_sidecar) {
        ddog_endpoint_drop(ddtrace_endpoint);
        ddog_sidecar_transport_drop(ddtrace_sidecar);
    }
}

void ddtrace_reset_sidecar_globals(void) {
    if (ddtrace_sidecar_instance_id) {
        ddog_sidecar_instanceId_drop(ddtrace_sidecar_instance_id);
        ddtrace_set_sidecar_globals();
    }
}

void ddtrace_sidecar_submit_root_span_data(void) {
    if (ddtrace_sidecar && DDTRACE_G(active_stack)) {
        ddtrace_span_data *root = DDTRACE_G(active_stack)->root_span;
        if (root) {
            zval *service = ddtrace_spandata_property_service(root);
            ddog_CharSlice service_slice = DDOG_CHARSLICE_C("");
            if (Z_TYPE_P(service) == IS_STRING) {
                service_slice = dd_zend_string_to_CharSlice(Z_STR_P(service));
            }

            zval *env = zend_hash_str_find(ddtrace_spandata_property_meta(root), ZEND_STRL("env"));
            ddog_CharSlice env_slice = DDOG_CHARSLICE_C("");
            if (env && Z_TYPE_P(env) == IS_STRING) {
                env_slice = dd_zend_string_to_CharSlice(Z_STR_P(env));
            }

            ddog_sidecar_set_remote_config_data(&ddtrace_sidecar, ddtrace_sidecar_instance_id, service_slice, env_slice, DDOG_CHARSLICE_C(""));
        }
    }
}
