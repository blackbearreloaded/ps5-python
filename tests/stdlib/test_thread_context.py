"""PS5 adaptations of CPython test_thread and test_contextvars."""

import _thread
import contextvars


done = _thread.allocate_lock()
done.acquire()
result = []
main_ident = _thread.get_ident()


def worker():
    result.append(_thread.get_ident() != main_ident)
    done.release()


thread = _thread.start_joinable_thread(worker)
done.acquire()
thread.join()
assert result == [True]

variable = contextvars.ContextVar("request_id", default="unset")
assert variable.get() == "unset"
token = variable.set("request-1")
assert variable.get() == "request-1"
context = contextvars.copy_context()
assert context.get(variable) == "request-1"
variable.reset(token)
assert variable.get() == "unset"

print("test_thread_context: PASS")
