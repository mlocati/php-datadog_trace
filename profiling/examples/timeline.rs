use datadog_php_profiling::config::AgentEndpoint;
use datadog_php_profiling::{
    profiling::{
        Label, LabelValue, ProfileIndex, Profiler, SampleValues, TimeCollector, WallTime, ZendFrame,
    },
    RequestLocals,
};
use datadog_profiling::profile;
use log::LevelFilter;
use memory_stats::memory_stats;
use std::collections::HashMap;
use std::sync::atomic::AtomicU32;
use std::time::Instant;

fn main() {
    let last_wall_export = WallTime::now();
    let mut profiles: HashMap<ProfileIndex, profile::Profile> = HashMap::with_capacity(1);

    let usage = memory_stats().unwrap();
    let start = Instant::now();

    let mut i = 0;
    let mut j = 0;
    while i < 5000000 {
        i += 10;
        // j = i / 10 >> 1;
        let sample_message = Profiler::bench_preapre_sample_message(
            get_frames(j),
            get_samples(i),
            // vec![],
            vec![Label {
                key: "end_timestamp_ns",
                value: LabelValue::Num(i, Some("nanoseconds")),
            }],
            &get_request_locals(),
        );
        // println!("{sample_message:?}");
        TimeCollector::handle_sample_message(sample_message, &mut profiles, &last_wall_export);
    }
    let duration = start.elapsed();

    println!("Walltime: {duration:?}");

    let new_usage = memory_stats().unwrap();
    println!("Memory:   {}", new_usage.physical_mem - usage.physical_mem);
}

#[inline]
fn get_frames(i: i64) -> Vec<ZendFrame> {
    vec![ZendFrame {
        function: "foobar()".into(),
        file: Some("foobar.php".into()),
        line: i as u32,
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
        // profiling_experimental_timeline_enabled: false,
        profiling_experimental_timeline_enabled: true,
        profiling_log_level: LevelFilter::Off,
        service: None,
        uri: Box::<AgentEndpoint>::default(),
        version: None,
        vm_interrupt_addr: std::ptr::null_mut(),
    }
}

#[inline]
fn get_samples(i: i64) -> SampleValues {
    SampleValues {
        interrupt_count: i + 1,
        wall_time: i + 2,
        cpu_time: i + 3,
        alloc_samples: i + 4,
        alloc_size: i + 5,
        // timeline: 0,
        timeline: i + 6,
    }
}
