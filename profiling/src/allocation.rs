use crate::bindings as zend;
use crate::PROFILER;
use crate::REQUEST_LOCALS;
use libc::c_void;
use libc::size_t;
use once_cell::sync::OnceCell;
use std::cell::RefCell;

use rand_distr::{Distribution, Poisson};

static JIT_ENABLED: OnceCell<bool> = OnceCell::new();

pub unsafe extern "C" fn ddog_alloc_profiling_register_destruct(
    ctx: *mut zend::zend_mm_observer_ctx,
) {
    let _ = Box::from_raw(ctx);
}

pub extern "C" fn ddog_allocation_profiling_register_observer() -> *mut zend::zend_mm_observer_ctx {
    let context = Box::new(zend::zend_mm_observer_ctx {
        malloc: Some(alloc_profiling_malloc),
        free: None,
        realloc: Some(alloc_profiling_realloc),
        destruct: Some(ddog_alloc_profiling_register_destruct),
    });

    Box::into_raw(context)
}

pub fn allocation_profiling_minit() {
    unsafe { zend::ddog_php_opcache_init_handle() };
    unsafe {
        zend::zend_mm_register_observer(Some(ddog_allocation_profiling_register_observer));
    }
}

/// take a sample every 2048 KB
pub const ALLOCATION_PROFILING_INTERVAL: f64 = 1024.0 * 2048.0;

pub struct AllocationProfilingStats {
    /// number of bytes until next sample collection
    next_sample: i64,
}

impl AllocationProfilingStats {
    fn new() -> AllocationProfilingStats {
        AllocationProfilingStats {
            next_sample: AllocationProfilingStats::next_sampling_interval(),
        }
    }

    fn next_sampling_interval() -> i64 {
        Poisson::new(ALLOCATION_PROFILING_INTERVAL)
            .unwrap()
            .sample(&mut rand::thread_rng()) as i64
    }

    fn track_allocation(&mut self, len: size_t) {
        self.next_sample -= len as i64;

        if self.next_sample > 0 {
            return;
        }

        self.next_sample = AllocationProfilingStats::next_sampling_interval();

        REQUEST_LOCALS.with(|cell| {
            // Panic: there might already be a mutable reference to `REQUEST_LOCALS`
            let locals = cell.try_borrow();
            if locals.is_err() {
                return;
            }
            let locals = locals.unwrap();

            if let Some(profiler) = PROFILER.lock().unwrap().as_ref() {
                // Safety: execute_data was provided by the engine, and the profiler doesn't mutate it.
                unsafe {
                    profiler.collect_allocations(
                        zend::ddog_php_prof_get_current_execute_data(),
                        1_i64,
                        len as i64,
                        &locals,
                    )
                };
            }
        });
    }
}

thread_local! {
    static ALLOCATION_PROFILING_STATS: RefCell<AllocationProfilingStats> = RefCell::new(AllocationProfilingStats::new());
}

unsafe extern "C" fn alloc_profiling_malloc(
    ctx: *mut zend::zend_mm_observer_ctx,
    len: size_t,
    ptr: *mut c_void,
) {
    // during startup, minit, rinit, ... current_execute_data is null
    // we are only interested in allocations during userland operations
    if zend::ddog_php_prof_get_current_execute_data().is_null() {
        return;
    }

    ALLOCATION_PROFILING_STATS.with(|cell| {
        let mut allocations = cell.borrow_mut();
        allocations.track_allocation(len)
    });
}

unsafe extern "C" fn alloc_profiling_realloc(
    ctx: *mut zend::zend_mm_observer_ctx,
    prev_ptr: *mut c_void,
    len: size_t,
    ptr: *mut c_void,
) {
    // during startup, minit, rinit, ... current_execute_data is null
    // we are only interested in allocations during userland operations
    if zend::ddog_php_prof_get_current_execute_data().is_null() || ptr == prev_ptr {
        return;
    }

    ALLOCATION_PROFILING_STATS.with(|cell| {
        let mut allocations = cell.borrow_mut();
        allocations.track_allocation(len)
    });
}
