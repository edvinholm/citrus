struct User_Server
{
    Atomic<bool> should_exit; // @Speed: Semaphore?
};
