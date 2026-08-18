# CPython upstream test basis

The language-core scripts in this directory are a PS5-compatible subset of
the official CPython regression tests under:

```text
upstream/cpython/Lib/test/
```

They keep the behavior being tested, but remove `unittest`, `test.support`,
filesystem helpers, subprocesses, and other standard-library dependencies so
the first PS5 runtime can execute them with no user imports. This is an
adaptation policy, not a replacement for CPython's complete regression suite.

## Coverage map

| PS5 test group | CPython source used as the behavior reference |
| --- | --- |
| `core_values/numeric.py` | `test_int.py`, `test_float.py`, `test_complex.py` |
| `core_values/comparisons.py` | `test_compare.py`, `test_richcmp.py` |
| `core_values/strings.py` | `test_str.py`, `test_string_literals.py` |
| `core_values/unicode.py` | `test_str.py`, `test_unicode_identifiers.py` |
| `core_values/lists.py` | `test_list.py` |
| `core_values/tuples.py` | `test_tuple.py` |
| `core_values/dictionaries.py` | `test_dict.py`, `test_dictcomps.py` |
| `core_values/sets.py` | `test_set.py`, `test_setcomps.py` |
| `core_objects/functions/*` | `test_funcattrs.py`, `test_class.py` |
| `core_objects/classes/*` | `test_class.py`, `test_subclassinit.py` |
| `core_objects/iteration/*` | `test_iter.py`, `test_generators.py`, `test_genexps.py`, `test_listcomps.py` |
| `core_control/exceptions.py` | `test_exceptions.py`, `test_baseexception.py` |
| `core_control/try_finally.py` | `test_exceptions.py` |
| `core_control/generators.py` | `test_generators.py`, `test_generator_stop.py` |
| `core_control/generator_expressions.py` | `test_genexps.py` |
| `core_control/iterators.py` | `test_iter.py`, `test_iterlen.py` |
| `core_control/gc_stress.py` | `test_gc.py` |
| `core_control/allocation_churn.py` | `test_list.py`, `test_dict.py`, `test_gc.py` |
| `stdlib/test_os.py` | `test_os.py` |
| `stdlib/test_time.py` | `test_time.py` |
| `stdlib/test_io.py` | `test_io.py` |
| `stdlib/test_socket.py` | `test_socket.py` |
| `stdlib/test_dns.py` | `test_socket.py` (`getaddrinfo`) |
| `stdlib/test_core_modules.py` | `test_re.py`, `test_json`, `test_struct`, `test_math`, `test_unicodedata` |
| `stdlib/test_ssl_hashlib.py` | `test_hashlib.py`, `test_ssl.py` |
| `stdlib/test_tls_handshake.py` | `test_ssl.py`, `test_socket.py` |
| `stdlib/test_thread_context.py` | `test_thread.py`, `test_contextvars.py` |
| `stdlib/test_concurrency.py` | `test_threading.py`, `test_concurrent_futures.py`, `_test_multiprocessing.py` |
| `stdlib/test_data_formats.py` | `test_csv.py`, `test_decimal.py`, `test_xml_etree.py` |
| `stdlib/test_import_runtime.py` | `test_pathlib.py`, `test_zipimport.py`, `test_stat.py` |
| `stdlib/test_filesystem.py` | `test_pathlib.py`, `test_tempfile.py` |
| `stdlib/test_tier1.py` | `test_sys.py`, `test_typing.py`, `datetimetester.py` |
| `stdlib/test_tier2.py` | `test_argparse.py`, `test_logging.py`, `test_shutil.py`, `test_random.py`, `test_copy.py`, `test_enum.py`, `test_csv.py`, `test_subprocess.py`, `test_urllib.py`, `test_hashlib.py`, `test_io.py`, `test_traceback.py`, `test_pprint.py`, `test_unittest.py` |
| `stdlib/test_diagnostics.py` | `test_tracemalloc.py`, `test_multiprocessing.py` |
| `stdlib/test_data_structures.py` | `test_collections.py`, `test_itertools.py`, `test_heapq.py`, `test_dataclasses.py` |
| `stdlib/test_profiling.py` | `test_timeit.py`, `test_dis.py`, `test_tracemalloc.py`, `test_struct.py` |
| `stdlib/test_posix_boundary.py` | `test_os.py`, `test_signal.py` |
| `stdlib/test_process.py` | `test_os.py` (`fork`, `waitpid`) |
| `stdlib/test_network.py` | `test_socket.py`, `test_select.py` (`poll`, non-blocking sockets) |
| `stdlib/test_select.py` | `test_select.py`, `test_selectors.py` |

The standard-library tests are intentionally direct, import-light adaptations
of the upstream behavior checks. They keep the upstream module-to-test naming
so each file can be compared directly with CPython. `time.sleep()` is excluded
because the current PS5 libc sleep syscall returns `ENOSYS`; it will be added
when the native sleep hook is implemented.

The exact upstream release is pinned by `CPYTHON_VERSION.txt`. When a test is
expanded, record the source file above and keep the PS5 version import-free.
The complete upstream suite remains a later host-validation phase after the
missing standard-library modules are ported.
