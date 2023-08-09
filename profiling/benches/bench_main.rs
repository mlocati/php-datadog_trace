use criterion::criterion_main;

mod benchmarks;

criterion_main!(
    benchmarks::timeline::timeline,
    benchmarks::stack_walking::stack_walking,
    // #[cfg(all(target_arch = "x86_64", target_os = "linux"))]
    // benchmarks::instructions_bench
);
