module;

#ifdef _WIN32
#include "Windows.h"
#endif

#include "utils.h"

export module window;
export import std.core;

export class IWindow : public NonCopyableAndMoveable
{
    inline static bool isWindowCreated{};

public:
    struct Event
    {
        struct Empty
        {

        };

        struct MousePosition
        {
            int x;
            int y;
        };

        struct MouseWheel
        {
            int yOffset;
        };

        struct WindowSize
        {
            int width;
            int height;
        };
    };
    using EventData = std::variant<
        Event::Empty,
        Event::MousePosition,
        Event::MouseWheel,
        Event::WindowSize
    >;

    IWindow(int width, int height) : mWidth{width}, mHeight{height}
    {
    }

    virtual ~IWindow()
    {
        isWindowCreated = false;
    };

    enum class Message
    {
#ifdef _WIN32
        EMPTY = WM_USER,
#else
        EMPTY
#endif // _WIN32
        RESIZE,
        QUIT,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_W_DOWN,
        KEY_W_UP,
        KEY_S_DOWN,
        KEY_S_UP,
        KEY_A_DOWN,
        KEY_A_UP,
        KEY_D_DOWN,
        KEY_D_UP,
        KEY_DELETE_DOWN,
        KEY_DELETE_UP,
        KEY_CTRL_DOWN,
        KEY_CTRL_UP,
        KEY_SHIFT_DOWN,
        KEY_SHIFT_UP,
        KEY_E_DOWN,
        KEY_E_UP,
        MOUSE_LEFT_DOWN,
        MOUSE_LEFT_UP,
        MOUSE_MIDDLE_DOWN,
        MOUSE_MIDDLE_UP,
        MOUSE_RIGHT_DOWN,
        MOUSE_RIGHT_UP,
        MOUSE_WHEEL,
        MOUSE_MOVE,
        COUNT,
    };

    virtual void init() = 0;
    virtual void show() = 0;
    virtual Message getMessage();
    virtual void putMessage(Message msg, EventData data = Event::Empty{})
    {
        mMessages.push(msg);
        mEventData.push(data);
    }

    [[nodiscard]] int getWidth() const
    {
        return mWidth;
    }
    [[nodiscard]] int getHeight() const
    {
        return mHeight;
    }

    void resize(int width, int height)
    {
        mWidth = width;
        mHeight = height;
    }

    [[nodiscard]] float getAspectRatio() const
    {
        return static_cast<float>(mWidth) / mHeight;
    }

    template <typename EventType>
    [[nodiscard]] EventType getEventData()
    {
        /*const */EventData data = mEventData.front();
        //ASSERT(std::holds_alternative<EventType>(data));

        // TODO: further investigate it
        while (!std::holds_alternative<EventType>(data))
        {
            WARN("TODO: stop/start processing mechanism seems to doesn't work properly");
            mEventData.pop();
            data = mEventData.front();
        }

        return std::get<EventType>(data);
    }

    void popEventData()
    {
        if (mEventData.size() > 0)
        {
            mEventData.pop();
        }
    }

    void resumeProcessing()
    {
        mIsProcessing = true;
    }

    void stopProcessing()
    {
        mIsProcessing = false;
    }

    static IWindow* createWindow(uint32_t width = 1280, uint32_t height = 720);

protected:
    std::queue<EventData> mEventData;
    std::queue<Message> mMessages;
    int mWidth;
    int mHeight;

    bool mIsProcessing = true;
};

module :private;

IWindow::Message IWindow::getMessage()
{
    Message msg = Message::EMPTY;
    if (!mMessages.empty())
    {
        msg = mMessages.front();
        mMessages.pop();
    }

    return msg;
}