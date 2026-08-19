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
| `stdlib/test_core_modules.py` | `test_re.py`, `test_json/test_encode_basestring_ascii.py`, `test_struct.py`, `test_math.py`, `test_unicodedata.py` |
| `stdlib/test_ssl_hashlib.py` | `test_hashlib.py`, `test_ssl.py` |
| `stdlib/test_tls_handshake.py` | `test_ssl.py`, `test_socket.py` |
| `stdlib/test_thread_context.py` | `test_thread.py`, `test_contextvars.py` |
| `stdlib/test_concurrency.py` | `test_threading.py`, `test_concurrent_futures/test_future.py`, `test_concurrent_futures/test_thread_pool.py`, `test_multiprocessing_fork/test_processes.py` |
| `stdlib/test_data_formats.py` | `test_csv.py`, `test_decimal.py`, `test_xml_etree.py` |
| `stdlib/test_import_runtime.py` | `test_pathlib/test_pathlib.py`, `test_zipfile/test_core.py`, `test_stat.py`, `test_importlib/test_api.py` |
| `stdlib/test_filesystem.py` | `test_pathlib/test_pathlib.py`, `test_tempfile.py` |
| `stdlib/test_sys.py` | `test_sys.py`, `test_typing.py`, `datetimetester.py` |
| `stdlib/test_argparse.py` | `test_argparse.py`, `test_logging.py`, `test_shutil.py`, `test_random.py`, `test_copy.py`, `test_enum.py`, `test_csv.py`, `test_subprocess.py`, `test_urllib.py`, `test_hashlib.py`, `test_io.py`, `test_traceback.py`, `test_pprint.py`, `test_unittest/` |
| `stdlib/test_threading.py` | `test_asyncio/test_tasks.py`, `test_threading.py`, `test_multiprocessing_fork/test_processes.py`, `test_concurrent_futures/test_future.py`, `test_socket.py`, `test_ssl.py`, `test_httpservers.py`, `test_queue.py`, `test_select.py`, `test_signal.py` |
| `stdlib/test_pickle.py` | `test_pickle.py`, `test_struct.py`, `test_bisect.py`, `test_heapq.py`, `test_array.py`, `test_fractions.py`, `test_base64.py`, `test_zlib.py`, `test_gzip.py`, `test_zipfile/test_core.py`, `test_tarfile.py`, `test_sqlite3/test_dbapi.py`, `test_xml_etree.py`, `test_xml_dom_minicompat.py`, `test_glob.py`, `test_fnmatch.py` |
| `stdlib/test_diagnostics.py` | `test_tracemalloc.py`, `test_mmap.py`, `test_multiprocessing_fork/test_processes.py` |
| `stdlib/test_data_structures.py` | `test_collections.py`, `test_itertools.py`, `test_heapq.py`, `test_dataclasses/test_dataclasses.py` |
| `stdlib/test_profiling.py` | `test_timeit.py`, `test_dis.py`, `test_tracemalloc.py`, `test_struct.py`, `test_profile.py`, `test_cprofile.py`, `test_pstats.py` |
| `stdlib/test_dis.py` | `test_ast/test_ast.py`, `test_dis.py`, `test_inspect/test_inspect.py` |
| `stdlib/test_contextlib.py` | `test_contextlib.py`, `test_gc.py`, `test_site.py`, `test_sysconfig.py`, `test_weakref.py`, `test_codecs.py`, `test_types.py` |
| `stdlib/test_secrets.py` | `test_secrets.py`, `test_hmac.py`, `test_getpass.py`, `test_gettext.py`, `test_locale.py`, `test_unicodedata.py` |
| `stdlib/test_filecmp.py` | `test_string/`, `test_textwrap.py`, `test_difflib.py`, `test_mimetypes.py`, `test_uuid.py`, `test_stat.py`, `test_filecmp.py` |
| `stdlib/test_fcntl.py` | `test_fcntl.py`, `test_resource.py`, `test_termios.py`, `test_tty.py` |
| `stdlib/test_code.py` | `test_doctest/test_doctest.py`, `test_py_compile.py`, `test_compileall.py`, `test_codeop.py`, `test_code.py` |
| `stdlib/test_code_module.py` | `test_code_module.py`, `test_pdb.py`, `test_readline.py`, `test_rlcompleter.py` (portable API subset) |
| `stdlib/test_graphlib.py` | `test_graphlib.py`, `test_statistics.py`, `test_cmath.py`, `test_ipaddress.py`, `test_colorsys.py`, `test_calendar.py`, `test_wave.py`, `test_binascii.py`, `test_zoneinfo/` (portable API subset) |
| `stdlib/test_bz2.py` | `test_bz2.py`, `test_lzma.py`, `test_dbm_dumb.py`, `test_shelve.py` (portable backend subset) |
| `stdlib/test_ftplib.py` | `test_ftplib.py`, `test_poplib.py`, `test_imaplib.py`, `test_smtplib.py`, `test_mailbox.py` (local protocol/mail subset) |
| `stdlib/test_shlex.py` | `test_shlex.py`, `test_getopt.py`, `test_optparse.py`, `test_cmd.py`, `test_symtable.py`, `test_pydoc/test_pydoc.py`, `test_webbrowser.py` (portable API subset) |
| `stdlib/test_marshal.py` | `test_future_stmt/`, `test_marshal.py`, `test_copyreg.py`, `test_thread.py` (portable core subset) |
| `stdlib/test_wsgiref.py` | `test_wsgiref.py` (loopback server, environment, validation, and HTTP response subset) |
| `stdlib/test_httpservers.py` | `test_wsgiref.py`, `test_httpservers.py` (ThreadingMixIn concurrency and shutdown subset) |
| `integration/test_gunicorn.py` | Third-party Gunicorn 23.0.0; CPython `test_wsgiref.py` and `test_httpservers.py` request-boundary basis |
| `integration/test_gunicorn_server.py` | Third-party Gunicorn 23.0.0; CPython `test_wsgiref.py`, `test_httpservers.py`, `test_os.py`, and `test_signal.py` lifecycle basis |
| `integration/test_gunicorn_foundation.py` | Third-party Gunicorn foundation; CPython `test_os.py`, `test_signal.py`, and `test_socket.py` process/socket basis |
| `integration/test_flask.py` | Third-party Flask stack; CPython `test_wsgiref.py`, `test_http_cookies.py`, and `test_http_cookiejar.py` WSGI/cookie basis |
| `stdlib/test_posix_boundary.py` | `test_os.py`, `test_signal.py` |
| `stdlib/test_process.py` | `test_os.py` (`fork`, `waitpid`) |
| `stdlib/test_worker_lifecycle.py` | `test_os.py`, `test_signal.py`, `test_socket.py` (forked listener/control-channel handoff, wait/restart, SIGCHLD, and TERM subset) |
| `stdlib/test_network.py` | `test_socket.py`, `test_select.py` (`poll`, non-blocking sockets) |
| `stdlib/test_select.py` | `test_select.py`, `test_selectors.py` |

The standard-library tests are intentionally direct, import-light adaptations
of the upstream behavior checks. Their filenames use an actual CPython
3.14.7 `Lib/test` filename (or the matching package path) rather than synthetic
`test_tier*.py` names. A few PS5 files are compact adapters whose primary
filename identifies the first upstream source while the table records every
additional source folded into that adapter; they can be split into those
upstream paths as coverage grows. `time.sleep()` is excluded
because the current PS5 libc sleep syscall returns `ENOSYS`; it will be added
when the native sleep hook is implemented.

The exact upstream release is pinned by `CPYTHON_VERSION.txt`. When a test is
expanded, record the source file above and keep the PS5 version import-free.
The complete upstream suite remains a later host-validation phase after the
missing standard-library modules are ported.
