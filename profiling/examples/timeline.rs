use clap::Parser;
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
use rand::Rng;
use std::collections::HashMap;
use std::sync::atomic::AtomicU32;
use std::time::Instant;

#[derive(Parser, Debug)]
struct Args {
    /// Enable timeline features by setting this flag
    #[arg(short, long, default_value_t = false)]
    timeline: bool,

    /// Number of samples to send
    #[arg(short, long, default_value_t = 500000)]
    count: i64,

    /// probability of stacks being identical
    /// 1.0: all stacks are identical
    /// 0.5: 50% of stacks are identical
    /// 0.0: no stacks are identical (100% unique stacks)
    #[arg(short, long, default_value_t = 1.0)]
    stacks: f64,
}

fn main() {
    let args = Args::parse();
    println!("{args:?}");

    if args.stacks > 1.0 {
        panic!("samples must be between 0.0 and 1.0");
    }

    let last_wall_export = WallTime::now();
    let mut profiles: HashMap<ProfileIndex, profile::Profile> = HashMap::with_capacity(1);

    let usage = memory_stats().unwrap();
    let start = Instant::now();
    let mut rng = rand::thread_rng();

    let mut i: i64 = 0;
    let mut j: i64;
    let len: i64 = args.count * 10;

    while i < len {
        i += 10;
        j = i;
        if rng.gen_range(0.0..1.0) >= args.stacks {
            j = 1;
        }
        let labels = if args.timeline {
            vec![Label {
                key: "end_timestamp_ns",
                value: LabelValue::Num(i, Some("nanoseconds")),
            }]
        } else {
            vec![]
        };
        let sample_message = Profiler::bench_preapre_sample_message(
            get_frames(j),
            get_samples(i, args.timeline),
            labels,
            &get_request_locals(args.timeline),
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
fn get_request_locals(timeline: bool) -> RequestLocals {
    RequestLocals {
        env: None,
        interrupt_count: AtomicU32::new(0),
        profiling_enabled: true,
        profiling_endpoint_collection_enabled: true,
        profiling_experimental_cpu_time_enabled: false,
        profiling_allocation_enabled: false,
        profiling_experimental_timeline_enabled: timeline,
        profiling_log_level: LevelFilter::Off,
        service: None,
        uri: Box::<AgentEndpoint>::default(),
        version: None,
        vm_interrupt_addr: std::ptr::null_mut(),
    }
}

#[inline]
fn get_samples(i: i64, timeline: bool) -> SampleValues {
    SampleValues {
        interrupt_count: i + 1,
        wall_time: i + 2,
        cpu_time: i + 3,
        alloc_samples: i + 4,
        alloc_size: i + 5,
        timeline: if timeline { i + 6 } else { 0 },
    }
}
