#!/usr/bin/env python3
"""
Universal CI test runner for all OpenNEC test suites.

Loads test suite definitions from CI_TEST_SUITES.json, executes each suite
with proper cleanup, timeouts, and reporting. Exit code 1 if any required
suite fails. Modular architecture enables adding new test suites by JSON
registration only (no CI workflow changes needed).

Usage:
  python3 tests/run_ci_tests.py --onec ./onec
  python3 tests/run_ci_tests.py --onec ./onec --suite golden-impedance --verbose
"""

import argparse
import glob
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).parent.parent
REGISTRY = HERE / "tests" / "CI_TEST_SUITES.json"


def expand_vars(template, binaries):
    """Expand {VAR} placeholders in config strings."""
    return template.format(
        ONEC_BIN=binaries.get("onec", "./onec"),
        TMPDIR=tempfile.gettempdir()
    )


def cleanup_suite(suite, verbose=False):
    """Clean up temporary files from test suite."""
    cleanup_count = 0
    for pattern in suite.get("cleanup_patterns", []):
        pattern_expanded = expand_vars(pattern, {})
        
        # Handle glob patterns
        for p in glob.glob(pattern_expanded):
            try:
                p_obj = Path(p)
                if p_obj.is_file():
                    p_obj.unlink()
                elif p_obj.is_dir():
                    shutil.rmtree(p)
                cleanup_count += 1
                if verbose:
                    print(f"    cleaned: {p}")
            except Exception as e:
                if verbose:
                    print(f"    cleanup warning: {p}: {e}")
    
    return cleanup_count


def run_suite(suite, binaries, verbose=False):
    """Execute one test suite, clean up, return (passed, returncode, output)."""
    suite_id = suite["id"]
    
    # Platform gate
    current_platform = platform.system()
    suite_platforms = [p.lower() for p in suite.get("platforms", ["linux", "darwin", "windows"])]
    
    if current_platform.lower() not in suite_platforms:
        print(f"  ⊘ {suite_id}: skipped (not on {suite['platforms']} platform)")
        return True, 0, ""
    
    # Prepare command
    runner_path = HERE / suite["runner"]
    if not runner_path.exists():
        print(f"  ✗ {suite_id}: runner not found at {runner_path}")
        return False, 1, f"Runner not found: {runner_path}"
    
    cmd = [str(runner_path)] + [expand_vars(arg, binaries) for arg in suite.get("args", [])]
    
    if verbose:
        print(f"  ▶ running: {' '.join(cmd)}")
    
    # Execute
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=suite.get("timeout_seconds", 300)
        )
    except subprocess.TimeoutExpired:
        print(f"  ✗ {suite_id}: timed out after {suite.get('timeout_seconds', 300)}s")
        cleanup_suite(suite, verbose)
        return False, 124, "Timeout"
    
    # Determine pass/fail
    expect_fail = suite.get("expect_failure", False)
    passed = (result.returncode == 0 and not expect_fail) or (result.returncode != 0 and expect_fail)
    
    # Cleanup
    cleanup_count = cleanup_suite(suite, verbose)
    if verbose and cleanup_count > 0:
        print(f"  (cleaned up {cleanup_count} file(s)/dir(s))")
    
    return passed, result.returncode, result.stdout + "\n" + result.stderr


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--onec", default="./onec", help="path to onec binary")
    ap.add_argument("--verbose", "-v", action="store_true", help="verbose output")
    ap.add_argument("--suite", help="run only this suite ID (for debugging)")
    args = ap.parse_args()
    
    # Load registry
    try:
        with open(REGISTRY) as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"ERROR: Registry not found: {REGISTRY}")
        return 1
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in {REGISTRY}: {e}")
        return 1
    
    binaries = {"onec": args.onec}
    
    # Verify binary exists
    if not Path(args.onec).exists():
        print(f"ERROR: onec binary not found: {args.onec}")
        return 1
    
    print(f"OpenNEC CI Test Suites ({len(config['test_suites'])} total)\n")
    
    failures = []
    skipped = 0
    
    for suite in config["test_suites"]:
        suite_id = suite["id"]
        
        # Filter by --suite if specified
        if args.suite and suite_id != args.suite:
            continue
        
        # Run suite
        passed, returncode, output = run_suite(suite, binaries, verbose=args.verbose)
        
        # Record results
        if suite.get("platforms", []):
            current_platform = platform.system()
            suite_platforms = [p.lower() for p in suite.get("platforms", [])]
            if current_platform.lower() not in suite_platforms:
                skipped += 1
                continue
        
        is_required = suite.get("required", True)
        is_expected_fail = suite.get("expect_failure", False)
        
        # Determine if test behaved as expected
        # - Required tests: pass when returncode==0
        # - Expected-to-fail tests: pass when returncode!=0
        test_succeeded = (returncode == 0) if not is_expected_fail else (returncode != 0)
        
        if not test_succeeded:
            if is_required:
                failures.append((suite_id, suite["name"], output))
            elif is_expected_fail and returncode == 0:
                # Expected to fail but test passed - unexpected progress!
                failures.append((suite_id, suite["name"] + " (passed unexpectedly)", output))
        
        # Print result
        if is_expected_fail:
            status = "✓" if returncode != 0 else "✗"
            expectation = " (expected red)" if returncode != 0 else " (unexpected pass!)"
        else:
            status = "✓" if returncode == 0 else "✗"
            expectation = ""
        
        print(f"{status} {suite_id}: {suite['name']}{expectation}")
        
        if args.verbose and not test_succeeded:
            output_preview = output[:300].replace('\n', '\n    ')
            print(f"    output: {output_preview}")
    
    print()
    
    # Summary
    total_required = len([s for s in config["test_suites"] if s.get("required", True)])
    
    if failures:
        print(f"✗ {len(failures)} suite(s) failed:")
        for sid, name, _ in failures:
            print(f"  ✗ {sid}: {name}")
        return 1
    
    print(f"✓ All {len(config['test_suites'])} suite(s) behaved as expected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
