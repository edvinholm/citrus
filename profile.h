
s64 frame_times[60*60] = {0};
u64 frame_time_cursor = 0;

void register_frame_time(s64 t) {
    frame_times[frame_time_cursor] = t;
    frame_time_cursor += 1;
    frame_time_cursor %= (sizeof(frame_times)/sizeof(frame_times[0]));
}
