

/*
  HOW TO USE A STANDARD LISTENING LOOP:
  1. To start the loop, call start_listening_loop().
     -----------------------------------------------
     Provide information about the socket you want to open,
     and which process you want to run in the thread you want to create
     to handle connecting clients.
     
  2. If loop.client_accept_failed == true, the loop is about to exit.
     ----------------------------------------------------------------
     so:
     * Wait for it with platform_join_thread(<your thread>).
     * deinit_listening_loop() -- This will close handles owned by the loop,
       including the listening socket.
     * If you want to restart the loop, call start_listening_loop again.
     
  3. When you want the loop to exit, call stop_listening_loop().
     -----------------------------------------------------------
     This will request the loop to exit, and wait for it to do so.
     You can pass a waiting timeout in ms.
     
  4. In the loop procedure
     ---------------------
     Loop with while(listening_loop_running()). If *_client_accepted is
     set to true after the call, we have accepted a new client and its socket
     handle has been written to *_client_socket.
 */

struct Listening_Loop
{
    Atomic<bool> should_exit;
    Atomic<bool> client_accept_failed;

    Socket socket;
};
