#pragma once
namespace tapdata
{
    class app;
    app* get_app();
    template <typename T>
    inline T* get_app()
    {
        return dynamic_cast<T*>(get_app());
    }

    class app
    {
        friend struct message_loop;

    public:
        app() noexcept;
        virtual ~app() noexcept;

        static void quit(int reason = 0) noexcept;
        static void notify_one() noexcept;
        static int exec(bool is_s = false) noexcept;
        static bool keep_run() noexcept;
    protected:
        virtual void run() noexcept = 0;
        //note construction problem
        virtual void sig_term_proc(int sig_no) = 0;

        //note construction problem
        virtual void sig_crash_proc(int sig_no) = 0;

        //note construction problem
        virtual void sig_dump_proc(int sig_no) = 0;
    };
}
