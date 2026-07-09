const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const old = b.option(u1, "old", "OLD compatibility flag (0 or 1)") orelse 0;

    // ── flags ─────────────────────────────────────────────────────────────

    var cpp_flags: std.ArrayList([]const u8) = .empty;
    cpp_flags.appendSlice(b.allocator, &.{
        "-std=c++17",
        "-Wall",
        "-Weffc++",
        "-Wextra",
        "-Werror",
        "-pedantic-errors",
        "-Wconversion",
        "-Wsign-conversion",
    }) catch @panic("OOM");
    if (optimize == .Debug) {
        cpp_flags.append(
            b.allocator,
            std.fmt.allocPrint(b.allocator, "-DOLD={d}", .{old}) catch @panic("OOM"),
        ) catch @panic("OOM");
    } else {
        cpp_flags.appendSlice(b.allocator, &.{ "-ffast-math", "-DNDEBUG" }) catch @panic("OOM");
    }

    const c_flags = &[_][]const u8{"-std=c11"};

    // ── include paths ─────────────────────────────────────────────────────

    const includes = [_][]const u8{
        "src",
        "engine/src",
        "engine/libs/dsp/include",
        "engine/libs/json/include",
        "libs/audio_io/include",
        "libs/audio_io/src",
        "libs/device_io/include",
        "libs/file_watch/include",
        "libs/file_watch/src",
        "libs/meh_utils/include",
        "deps/lua/include",
        "deps/linenoise",
        "deps/imgui",
        "deps/imgui/backends",
        "deps/glfw/include",
    };

    // ── source directories ────────────────────────────────────────────────

    const app_cpp_dirs = [_][]const u8{
        "src",
        "engine/src",
        "engine/libs/dsp/src",
        "engine/libs/json/src",
        "libs/audio_io/src",
        "libs/device_io/src",
        "libs/file_watch/src",
        "deps/imgui",
    };

    const c_dirs = [_][]const u8{
        "deps/lua/src",
        "deps/linenoise",
    };

    const frameworks = [_][]const u8{
        "AudioToolbox", "CoreAudio", "CoreFoundation", "CoreMIDI",
        "CoreServices", "OpenGL",    "Cocoa",          "ApplicationServices",
        "IOKit",
    };

    // ── main executable ───────────────────────────────────────────────────

    const main_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    for (includes) |inc| main_mod.addIncludePath(b.path(inc));
    for (frameworks) |fw| main_mod.linkFramework(fw, .{});
    main_mod.addLibraryPath(b.path("deps/glfw/lib"));
    main_mod.linkSystemLibrary("glfw3", .{});

    var cpp_srcs: std.ArrayList([]const u8) = .empty;
    for (app_cpp_dirs) |dir| collectSources(b, &cpp_srcs, dir, ".cpp", null);
    main_mod.addCSourceFiles(.{ .files = cpp_srcs.items, .flags = cpp_flags.items });

    var c_srcs: std.ArrayList([]const u8) = .empty;
    for (c_dirs) |dir| collectSources(b, &c_srcs, dir, ".c", null);
    main_mod.addCSourceFiles(.{ .files = c_srcs.items, .flags = c_flags });

    const exe = b.addExecutable(.{ .name = "main", .root_module = main_mod });
    b.installArtifact(exe);

    b.step("run", "Run the app").dependOn(&b.addRunArtifact(exe).step);

    // ── test runner ───────────────────────────────────────────────────────

    const test_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    for (includes) |inc| test_mod.addIncludePath(b.path(inc));
    test_mod.addIncludePath(b.path("tests"));
    for (frameworks) |fw| test_mod.linkFramework(fw, .{});
    test_mod.addLibraryPath(b.path("deps/glfw/lib"));
    test_mod.linkSystemLibrary("glfw3", .{});

    var test_cpp_srcs: std.ArrayList([]const u8) = .empty;
    for (app_cpp_dirs) |dir| collectSources(b, &test_cpp_srcs, dir, ".cpp", "src/main.cpp");
    collectSources(b, &test_cpp_srcs, "tests", ".cpp", null);
    test_mod.addCSourceFiles(.{ .files = test_cpp_srcs.items, .flags = cpp_flags.items });

    var test_c_srcs: std.ArrayList([]const u8) = .empty;
    for (c_dirs) |dir| collectSources(b, &test_c_srcs, dir, ".c", null);
    test_mod.addCSourceFiles(.{ .files = test_c_srcs.items, .flags = c_flags });

    const test_exe = b.addExecutable(.{ .name = "test_runner", .root_module = test_mod });
    b.installArtifact(test_exe);

    b.step("test", "Build and run tests").dependOn(&b.addRunArtifact(test_exe).step);

    // ── luals stub generator ──────────────────────────────────────────────

    const luals_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    for (includes) |inc| luals_mod.addIncludePath(b.path(inc));
    luals_mod.addCSourceFiles(.{
        .files = &.{
            "src/app/AppParams.cpp",
            "src/app/doc/DocMetadata.cpp",
            "src/lua/metadata/LuaRuntimeMetadata.cpp",
            "tools/luals/generate_luals_stubs.cpp",
        },
        .flags = cpp_flags.items,
    });

    const luals_exe = b.addExecutable(.{ .name = "generate_luals_stubs", .root_module = luals_mod });
    b.installArtifact(luals_exe);

    const luals_run = b.addRunArtifact(luals_exe);
    luals_run.addArgs(&.{ "--out", "generated/luals" });
    b.step("luals-stubs", "Generate LuaLS stubs").dependOn(&luals_run.step);

    const luals_check = b.addRunArtifact(luals_exe);
    luals_check.addArgs(&.{ "--out", "generated/luals", "--check" });
    b.step("check-luals-stubs", "Check LuaLS stubs").dependOn(&luals_check.step);
}

fn collectSources(
    b: *std.Build,
    list: *std.ArrayList([]const u8),
    dir_path: []const u8,
    ext: []const u8,
    exclude: ?[]const u8,
) void {
    var dir = b.build_root.handle.openDir(b.graph.io, dir_path, .{ .iterate = true }) catch return;
    defer dir.close(b.graph.io);
    var walker = dir.walk(b.allocator) catch return;
    defer walker.deinit();
    while (walker.next(b.graph.io) catch return) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.path, ext)) continue;
        const full = std.fs.path.join(b.allocator, &.{ dir_path, entry.path }) catch @panic("OOM");
        if (exclude) |ex| if (std.mem.eql(u8, full, ex)) continue;
        list.append(b.allocator, full) catch @panic("OOM");
    }
}
