version 14
set more off
adopath + ../src

program main
    test_good
    test_bad
    exit, clear STATA
end

program test_good
    // test basic functionality. Write CSV to confirm that Python gets back what
    // it writes
    shm_use using ../temp/test_segment_info.txt, clear
    format %18.17f float_var
    outsheet using ../temp/results_from_stata.csv, comma replace

    // test compression option to demote variable types where possible
    shm_use using ../temp/test_segment_info.txt, clear compress

    // test the deallocate option to free memory
    shm_use using ../temp/test_segment_info.txt, clear deallocate
    display "Testing deallocation: "
    shell ipcs
end

program test_bad
    // test that reading segments of variable size fails without allocating memory
    capture noisily shm_use using test_bad_segments.txt, clear
    assert _rc == 3598
end

* EXECUTE
main
