// Unless explicitly stated otherwise all files in this repository are licensed under the Apache License Version 2.0.
// This product includes software developed at Datadog (https://www.datadoghq.com/). Copyright 2021-Present Datadog, Inc.

#ifndef DDOG_COMMON_H
#define DDOG_COMMON_H

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define DDOG_CHARSLICE_C(string) \
/* NOTE: Compilation fails if you pass in a char* instead of a literal */ {.ptr = "" string, .len = sizeof(string) - 1}
#else
#define DDOG_CHARSLICE_C(string) \
/* NOTE: Compilation fails if you pass in a char* instead of a literal */ ((ddog_CharSlice){ .ptr = "" string, .len = sizeof(string) - 1 })
#endif

#if defined __GNUC__
#  define DDOG_GNUC_VERSION(major) __GNUC__ >= major
#else
#  define DDOG_GNUC_VERSION(major) (0)
#endif

#if defined __has_attribute
#  define DDOG_HAS_ATTRIBUTE(attribute, major) __has_attribute(attribute)
#else
#  define DDOG_HAS_ATTRIBUTE(attribute, major) DDOG_GNUC_VERSION(major)
#endif

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#  define DDOG_CHECK_RETURN [[nodiscard]]
#elif defined(_Check_return_) /* SAL */
#  define DDOG_CHECK_RETURN _Check_return_
#elif DDOG_HAS_ATTRIBUTE(warn_unused_result, 4)
#  define DDOG_CHECK_RETURN __attribute__((__warn_unused_result__))
#else
#  define DDOG_CHECK_RETURN
#endif

typedef struct ddog_Endpoint ddog_Endpoint;

typedef struct ddog_Tag ddog_Tag;

/**
 * Holds the raw parts of a Rust Vec; it should only be created from Rust,
 * never from C.
 */
typedef struct ddog_Vec_U8 {
  const uint8_t *ptr;
  uintptr_t len;
  uintptr_t capacity;
} ddog_Vec_U8;

/**
 * Please treat this as opaque; do not reach into it, and especially don't
 * write into it! The most relevant APIs are:
 * * `ddog_Error_message`, to get the message as a slice.
 * * `ddog_Error_drop`.
 */
typedef struct ddog_Error {
  /**
   * This is a String stuffed into the vec.
   */
  struct ddog_Vec_U8 message;
} ddog_Error;

/**
 * Remember, the data inside of each member is potentially coming from FFI,
 * so every operation on it is unsafe!
 */
typedef struct ddog_Slice_CChar {
  const char *ptr;
  uintptr_t len;
} ddog_Slice_CChar;

typedef struct ddog_Slice_CChar ddog_CharSlice;

/**
 * Holds the raw parts of a Rust Vec; it should only be created from Rust,
 * never from C.
 */
typedef struct ddog_Vec_Tag {
  const struct ddog_Tag *ptr;
  uintptr_t len;
  uintptr_t capacity;
} ddog_Vec_Tag;

typedef enum ddog_Vec_Tag_PushResult_Tag {
  DDOG_VEC_TAG_PUSH_RESULT_OK,
  DDOG_VEC_TAG_PUSH_RESULT_ERR,
} ddog_Vec_Tag_PushResult_Tag;

typedef struct ddog_Vec_Tag_PushResult {
  ddog_Vec_Tag_PushResult_Tag tag;
  union {
    struct {
      struct ddog_Error err;
    };
  };
} ddog_Vec_Tag_PushResult;

typedef struct ddog_Vec_Tag_ParseResult {
  struct ddog_Vec_Tag tags;
  struct ddog_Error *error_message;
} ddog_Vec_Tag_ParseResult;

typedef enum ddog_ConfigurationOrigin {
  DDOG_CONFIGURATION_ORIGIN_ENV_VAR,
  DDOG_CONFIGURATION_ORIGIN_CODE,
  DDOG_CONFIGURATION_ORIGIN_DD_CONFIG,
  DDOG_CONFIGURATION_ORIGIN_REMOTE_CONFIG,
  DDOG_CONFIGURATION_ORIGIN_DEFAULT,
} ddog_ConfigurationOrigin;

typedef enum ddog_InBodyLocation {
  DDOG_IN_BODY_LOCATION_NONE,
  DDOG_IN_BODY_LOCATION_START,
  DDOG_IN_BODY_LOCATION_END,
} ddog_InBodyLocation;

typedef struct ddog_BlockingTransport_SidecarInterfaceResponse__SidecarInterfaceRequest ddog_BlockingTransport_SidecarInterfaceResponse__SidecarInterfaceRequest;

typedef struct ddog_InstanceId ddog_InstanceId;

typedef struct ddog_RemoteConfigState ddog_RemoteConfigState;

typedef struct ddog_TelemetryActionsBuffer ddog_TelemetryActionsBuffer;

typedef struct ddog_Log {
  uint32_t bits;
} ddog_Log;
typedef struct ddog_BlockingTransport_SidecarInterfaceResponse__SidecarInterfaceRequest ddog_SidecarTransport;

typedef enum ddog_Option_VecU8_Tag {
  DDOG_OPTION_VEC_U8_SOME_VEC_U8,
  DDOG_OPTION_VEC_U8_NONE_VEC_U8,
} ddog_Option_VecU8_Tag;

typedef struct ddog_Option_VecU8 {
  ddog_Option_VecU8_Tag tag;
  union {
    struct {
      struct ddog_Vec_U8 some;
    };
  };
} ddog_Option_VecU8;

typedef struct ddog_Option_VecU8 ddog_MaybeError;

typedef enum ddog_IntermediateValue_Tag {
  DDOG_INTERMEDIATE_VALUE_STRING,
  DDOG_INTERMEDIATE_VALUE_NUMBER,
  DDOG_INTERMEDIATE_VALUE_BOOL,
  DDOG_INTERMEDIATE_VALUE_NULL,
  DDOG_INTERMEDIATE_VALUE_REFERENCED,
} ddog_IntermediateValue_Tag;

typedef struct ddog_IntermediateValue {
  ddog_IntermediateValue_Tag tag;
  union {
    struct {
      ddog_CharSlice string;
    };
    struct {
      double number;
    };
    struct {
      bool bool_;
    };
    struct {
      const void *referenced;
    };
  };
} ddog_IntermediateValue;

typedef struct ddog_VoidCollection {
  intptr_t count;
  const void *elements;
  void (*free)(struct ddog_VoidCollection);
} ddog_VoidCollection;

typedef struct ddog_Evaluator {
  bool (*equals)(const void*, struct ddog_IntermediateValue, struct ddog_IntermediateValue);
  bool (*greater_than)(const void*, struct ddog_IntermediateValue, struct ddog_IntermediateValue);
  bool (*greater_or_equals)(const void*,
                            struct ddog_IntermediateValue,
                            struct ddog_IntermediateValue);
  const void *(*fetch_identifier)(const void*, const ddog_CharSlice*);
  const void *(*fetch_index)(const void*, const void*, struct ddog_IntermediateValue);
  const void *(*fetch_nested)(const void*, const void*, struct ddog_IntermediateValue);
  uint64_t (*length)(const void*, const void*);
  struct ddog_VoidCollection (*try_enumerate)(const void*, const void*);
  struct ddog_VoidCollection (*stringify)(const void*, const void*);
  intptr_t (*convert_index)(const void*, const void*);
} ddog_Evaluator;

typedef enum ddog_Option_CharSlice_Tag {
  DDOG_OPTION_CHAR_SLICE_SOME_CHAR_SLICE,
  DDOG_OPTION_CHAR_SLICE_NONE_CHAR_SLICE,
} ddog_Option_CharSlice_Tag;

typedef struct ddog_Option_CharSlice {
  ddog_Option_CharSlice_Tag tag;
  union {
    struct {
      ddog_CharSlice some;
    };
  };
} ddog_Option_CharSlice;

typedef struct ddog_CharSliceVec {
  const ddog_CharSlice *strings;
  uintptr_t string_count;
} ddog_CharSliceVec;

typedef struct ddog_ProbeTarget {
  struct ddog_Option_CharSlice type_name;
  struct ddog_Option_CharSlice method_name;
  struct ddog_Option_CharSlice source_file;
  struct ddog_Option_CharSlice signature;
  struct ddog_CharSliceVec lines;
  enum ddog_InBodyLocation in_body_location;
} ddog_ProbeTarget;

typedef struct ddog_LiveDebuggerCallbacks {
  int64_t (*set_span_probe)(const struct ddog_ProbeTarget *target);
  void (*remove_span_probe)(int64_t id);
} ddog_LiveDebuggerCallbacks;

typedef struct ddog_LiveDebuggerSetup {
  const struct ddog_Evaluator *evaluator;
  struct ddog_LiveDebuggerCallbacks callbacks;
} ddog_LiveDebuggerSetup;

typedef enum ddog_EvaluateAt {
  DDOG_EVALUATE_AT_ENTRY,
  DDOG_EVALUATE_AT_EXIT,
} ddog_EvaluateAt;

typedef enum ddog_MetricKind {
  DDOG_METRIC_KIND_COUNT,
  DDOG_METRIC_KIND_GAUGE,
  DDOG_METRIC_KIND_HISTOGRAM,
  DDOG_METRIC_KIND_DISTRIBUTION,
} ddog_MetricKind;

typedef enum ddog_SpanProbeTarget {
  DDOG_SPAN_PROBE_TARGET_ACTIVE,
  DDOG_SPAN_PROBE_TARGET_ROOT,
} ddog_SpanProbeTarget;

typedef struct ddog_DslString ddog_DslString;

typedef struct ddog_ProbeCondition ddog_ProbeCondition;

typedef struct ddog_ProbeValue ddog_ProbeValue;

typedef struct ddog_MetricProbe {
  enum ddog_MetricKind kind;
  ddog_CharSlice name;
  const struct ddog_ProbeValue *value;
} ddog_MetricProbe;

typedef struct ddog_Capture {
  uint32_t max_reference_depth;
  uint32_t max_collection_size;
  uint32_t max_length;
  uint32_t max_field_depth;
} ddog_Capture;

typedef struct ddog_LogProbe {
  const struct ddog_DslString *segments;
  const struct ddog_ProbeCondition *when;
  const struct ddog_Capture *capture;
  uint32_t sampling_snapshots_per_second;
} ddog_LogProbe;

typedef struct ddog_Tag {
  ddog_CharSlice name;
  const struct ddog_DslString *value;
} ddog_Tag;

typedef struct ddog_SpanProbeDecoration {
  const struct ddog_ProbeCondition *condition;
  const struct ddog_Tag *tags;
  uintptr_t tags_count;
} ddog_SpanProbeDecoration;

typedef struct ddog_SpanDecorationProbe {
  enum ddog_SpanProbeTarget target;
  const struct ddog_SpanProbeDecoration *decorations;
  uintptr_t decorations_count;
} ddog_SpanDecorationProbe;

typedef enum ddog_ProbeType_Tag {
  DDOG_PROBE_TYPE_METRIC,
  DDOG_PROBE_TYPE_LOG,
  DDOG_PROBE_TYPE_SPAN,
  DDOG_PROBE_TYPE_SPAN_DECORATION,
} ddog_ProbeType_Tag;

typedef struct ddog_ProbeType {
  ddog_ProbeType_Tag tag;
  union {
    struct {
      struct ddog_MetricProbe metric;
    };
    struct {
      struct ddog_LogProbe log;
    };
    struct {
      struct ddog_SpanDecorationProbe span_decoration;
    };
  };
} ddog_ProbeType;

typedef struct ddog_Probe {
  ddog_CharSlice id;
  uint64_t version;
  struct ddog_Option_CharSlice language;
  struct ddog_CharSliceVec tags;
  struct ddog_ProbeTarget target;
  enum ddog_EvaluateAt evaluate_at;
  struct ddog_ProbeType probe;
} ddog_Probe;

typedef struct ddog_FilterList {
  struct ddog_CharSliceVec package_prefixes;
  struct ddog_CharSliceVec classes;
} ddog_FilterList;

typedef struct ddog_ServiceConfiguration {
  ddog_CharSlice id;
  struct ddog_FilterList allow;
  struct ddog_FilterList deny;
  uint32_t sampling_snapshots_per_second;
} ddog_ServiceConfiguration;

typedef enum ddog_LiveDebuggingData_Tag {
  DDOG_LIVE_DEBUGGING_DATA_NONE,
  DDOG_LIVE_DEBUGGING_DATA_PROBE,
  DDOG_LIVE_DEBUGGING_DATA_SERVICE_CONFIGURATION,
} ddog_LiveDebuggingData_Tag;

typedef struct ddog_LiveDebuggingData {
  ddog_LiveDebuggingData_Tag tag;
  union {
    struct {
      struct ddog_Probe probe;
    };
    struct {
      struct ddog_ServiceConfiguration service_configuration;
    };
  };
} ddog_LiveDebuggingData;

typedef struct ddog_LiveDebuggingParseResult {
  struct ddog_LiveDebuggingData data;
  struct ddog_LiveDebuggingData *opaque_data;
} ddog_LiveDebuggingParseResult;

typedef enum ddog_LogLevel {
  DDOG_LOG_LEVEL_ERROR,
  DDOG_LOG_LEVEL_WARN,
  DDOG_LOG_LEVEL_DEBUG,
} ddog_LogLevel;

typedef struct ddog_TelemetryWorkerBuilder ddog_TelemetryWorkerBuilder;

typedef struct ddog_TelemetryWorkerHandle ddog_TelemetryWorkerHandle;

typedef enum ddog_Option_Bool_Tag {
  DDOG_OPTION_BOOL_SOME_BOOL,
  DDOG_OPTION_BOOL_NONE_BOOL,
} ddog_Option_Bool_Tag;

typedef struct ddog_Option_Bool {
  ddog_Option_Bool_Tag tag;
  union {
    struct {
      bool some;
    };
  };
} ddog_Option_Bool;

typedef struct ddog_AgentRemoteConfigReader ddog_AgentRemoteConfigReader;

typedef struct ddog_AgentRemoteConfigWriter_ShmHandle ddog_AgentRemoteConfigWriter_ShmHandle;

typedef struct ddog_MappedMem_ShmHandle ddog_MappedMem_ShmHandle;

/**
 * PlatformHandle contains a valid reference counted FileDescriptor and associated Type information
 * allowing safe transfer and sharing of file handles across processes, and threads
 */
typedef struct ddog_PlatformHandle_File ddog_PlatformHandle_File;

typedef struct ddog_RemoteConfigReader ddog_RemoteConfigReader;

typedef struct ddog_RuntimeMeta ddog_RuntimeMeta;

typedef struct ddog_ShmHandle ddog_ShmHandle;

typedef struct ddog_NativeFile {
  struct ddog_PlatformHandle_File *handle;
} ddog_NativeFile;

typedef struct ddog_TracerHeaderTags {
  ddog_CharSlice lang;
  ddog_CharSlice lang_version;
  ddog_CharSlice lang_interpreter;
  ddog_CharSlice lang_vendor;
  ddog_CharSlice tracer_version;
  ddog_CharSlice container_id;
  bool client_computed_top_level;
  bool client_computed_stats;
} ddog_TracerHeaderTags;

/**
 * # Safety
 * Only pass null or a valid reference to a `ddog_Error`.
 */
void ddog_Error_drop(struct ddog_Error *error);

/**
 * Returns a CharSlice of the error's message that is valid until the error
 * is dropped.
 * # Safety
 * Only pass null or a valid reference to a `ddog_Error`.
 */
ddog_CharSlice ddog_Error_message(const struct ddog_Error *error);

DDOG_CHECK_RETURN struct ddog_Endpoint *ddog_endpoint_from_url(ddog_CharSlice url);

DDOG_CHECK_RETURN struct ddog_Endpoint *ddog_endpoint_from_api_key(ddog_CharSlice api_key);

DDOG_CHECK_RETURN
struct ddog_Error *ddog_endpoint_from_api_key_and_site(ddog_CharSlice api_key,
                                                       ddog_CharSlice site,
                                                       struct ddog_Endpoint **endpoint);

void ddog_endpoint_drop(struct ddog_Endpoint*);

DDOG_CHECK_RETURN struct ddog_Vec_Tag ddog_Vec_Tag_new(void);

void ddog_Vec_Tag_drop(struct ddog_Vec_Tag);

/**
 * Creates a new Tag from the provided `key` and `value` by doing a utf8
 * lossy conversion, and pushes into the `vec`. The strings `key` and `value`
 * are cloned to avoid FFI lifetime issues.
 *
 * # Safety
 * The `vec` must be a valid reference.
 * The CharSlices `key` and `value` must point to at least many bytes as their
 * `.len` properties claim.
 */
DDOG_CHECK_RETURN
struct ddog_Vec_Tag_PushResult ddog_Vec_Tag_push(struct ddog_Vec_Tag *vec,
                                                 ddog_CharSlice key,
                                                 ddog_CharSlice value);

/**
 * # Safety
 * The `string`'s .ptr must point to a valid object at least as large as its
 * .len property.
 */
DDOG_CHECK_RETURN struct ddog_Vec_Tag_ParseResult ddog_Vec_Tag_parse(ddog_CharSlice string);

#endif /* DDOG_COMMON_H */
