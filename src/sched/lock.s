    .global k_lock_release
    .type   k_lock_release, @function
    .text
k_lock_release:
    sti
    ret

    .global k_lock
    .type   k_lock, @function
    .text
k_lock:
    cli
    ret

