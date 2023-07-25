use crate::bindings as zend;
use crate::{PROFILER, REQUEST_LOCALS};
use libc::{c_void, size_t};
use log::error;
use once_cell::sync::OnceCell;
use std::cell::RefCell;
use std::ptr::null_mut;

use rand_distr::{Distribution, Poisson};

static JIT_ENABLED: OnceCell<bool> = OnceCell::new();
thread_local! {
    static OBSERVER: RefCell<*mut zend::zend_mm_observer> = RefCell::new(std::ptr::null_mut());
}

pub fn allocation_profiling_minit() {
    unsafe {
        zend::ddog_php_opcache_init_handle();
    }
}

pub fn allocation_profiling_rinit() {
    let jit = JIT_ENABLED.get_or_init(|| unsafe { zend::ddog_php_jit_enabled() });
    if *jit {
        error!("Memory allocation profiling will be disabled as long as JIT is active. To enable allocation profiling disable JIT. See https://github.com/DataDog/dd-trace-php/pull/2088");
        REQUEST_LOCALS.with(|cell| {
            let mut locals = cell.borrow_mut();
            locals.profiling_allocation_enabled = false;
        });
    }

    REQUEST_LOCALS.with(|cell| {
        let locals = cell.borrow();
        if locals.profiling_allocation_enabled {
            OBSERVER.with(|observer| {
                *observer.borrow_mut() = unsafe {
                    zend::zend_mm_observer_register(
                        null_mut(),
                        Some(alloc_profiling_malloc),
                        None,
                        Some(alloc_profiling_realloc),
                    )
                };
            });
        }
    });
}

pub fn allocation_profiling_rshutdown() {
    OBSERVER.with(|observer| {
        let observer = *observer.borrow();
        if !observer.is_null() {
            unsafe { zend::zend_mm_observer_unregister(null_mut(), observer) };
        }
    });
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

unsafe extern "C" fn alloc_profiling_malloc(len: size_t, _ptr: *mut c_void) {
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

unsafe extern "C" fn alloc_profiling_realloc(prev_ptr: *mut c_void, len: size_t, ptr: *mut c_void) {
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
