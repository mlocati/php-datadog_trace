use datadog_php_profiling::config::AgentEndpoint;
use datadog_php_profiling::profiling::ProfileIndex;
use datadog_php_profiling::profiling::Profiler;
use datadog_php_profiling::profiling::SampleValues;
use datadog_php_profiling::profiling::TimeCollector;
use datadog_php_profiling::profiling::WallTime;
use datadog_php_profiling::profiling::ZendFrame;
use datadog_php_profiling::RequestLocals;
use datadog_profiling::profile;
use log::LevelFilter;
use memory_stats::memory_stats;
use std::collections::HashMap;
use std::sync::atomic::AtomicU32;

fn main() {
    let last_wall_export = WallTime::now();
    let mut profiles: HashMap<ProfileIndex, profile::Profile> = HashMap::with_capacity(1);

    if let Some(usage) = memory_stats() {
        println!("memory usage: {usage:?}");
    }

    let mut i = 0;
    while i < 9999999 {
        i += 1;
        let sample_message = Profiler::bench_preapre_sample_message(
            get_frames(),
            get_samples(),
            vec![],
            &get_request_locals(),
        );
        TimeCollector::handle_sample_message(sample_message, &mut profiles, &last_wall_export);
    }

    if let Some(usage) = memory_stats() {
        println!("memory usage: {usage:?}");
        println!("length: {}", profiles.len());
    }
}

#[inline]
fn get_frames() -> Vec<ZendFrame> {
    vec![ZendFrame {
        function: "foobar()".into(),
        file: Some("foobar.php".into()),
        line: 42,
    }]
}

#[inline]
fn get_request_locals() -> RequestLocals {
    RequestLocals {
        env: None,
        interrupt_count: AtomicU32::new(0),
        profiling_enabled: true,
        profiling_endpoint_collection_enabled: true,
        profiling_experimental_cpu_time_enabled: false,
        profiling_allocation_enabled: false,
        profiling_experimental_timeline_enabled: false,
        profiling_log_level: LevelFilter::Off,
        service: None,
        uri: Box::<AgentEndpoint>::default(),
        version: None,
        vm_interrupt_addr: std::ptr::null_mut(),
    }
}

#[inline]
fn get_samples() -> SampleValues {
    SampleValues {
        interrupt_count: 10,
        wall_time: 20,
        cpu_time: 30,
        alloc_samples: 40,
        alloc_size: 50,
        timeline: 60,
    }
}
