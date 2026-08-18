"""Run every language-core test from the external PS5 test bundle."""

TEST_FILES = (
    "core_basics.py",
    "allocation_churn.py",
    "exceptions.py",
    "gc_stress.py",
    "generator_expressions.py",
    "generators.py",
    "iterators.py",
    "try_finally.py",
    "attribute_lookup.py",
    "classes_basic.py",
    "inheritance_methods.py",
    "closures_recursion.py",
    "functions.py",
    "comprehensions.py",
    "for_iteration.py",
    "comparisons.py",
    "lists.py",
    "numeric.py",
    "tuples.py",
    "dictionaries.py",
    "sets.py",
    "strings.py",
    "unicode.py",
    "test_os.py",
    "test_time.py",
    "test_io.py",
    "test_socket.py",
    "test_dns.py",
    "test_core_modules.py",
    "test_ssl_hashlib.py",
    "test_thread_context.py",
    "test_data_formats.py",
    "test_posix_boundary.py",
    "test_process.py",
    "test_network.py",
    "test_select.py",
    "test_selectors.py",
)

for filename in TEST_FILES:
    path = "/data/python/core-tests/" + filename
    print("RUN", filename, flush=True)
    source = open(path, "rb").read()
    namespace = {"__name__": "__main__", "__file__": path}
    exec(compile(source, path, "exec"), namespace, namespace)

print("CPYTHON_CORE_SUITE: PASS (%d scripts)" % len(TEST_FILES), flush=True)
