module;

#ifdef _WIN32
#include "Windows.h"
#endif

#include "utils.h"

export module window;
export import std.core;

export class IWindow : public NonCopyableAndNonMoveable
{
    inline static bool isWindowCreated{};

public:
    struct Event
    {
        struct MousePosition
        {
            int xoffset;
            int yoffset;
        };

        struct MouseWheel
        {
            int yoffset;
        };
    };
    // clang-format off
    using EventData = std::variant<
        Event::MousePosition,
        Event::MouseWheel
    >;
    // clang-format

    IWindow(const int width = 1280, const int height = 720) : mWidth{width}, mHeight{height}
    {
    }

    virtual ~IWindow()
    {
        isWindowCreated = false;
    };

    enum class Message
    {
#ifdef _WIN32
        RESIZE = WM_USER + 1,
#else
        RESIZE,
#error not implemented
#endif // _WIN32
        QUIT,
        KEY_W_DOWN,
        KEY_W_UP,
        KEY_S_DOWN,
        KEY_S_UP,
        KEY_A_DOWN,
        KEY_A_UP,
        KEY_D_DOWN,
        KEY_D_UP,
        MOUSE_LEFT_DOWN,
        MOUSE_LEFT_UP,
        MOUSE_RIGHT_DOWN,
        MOUSE_RIGHT_UP,
        MOUSE_MIDDLE_DOWN,
        MOUSE_MIDDLE_UP,
        MOUSE_WHEEL,
        MOUSE_MOVE,
    };

    virtual void init() = 0;
    virtual void show() = 0;
    virtual Message peekMessage() = 0;
    virtual void dispatchMessage() = 0;

    [[nodiscard]] int getWidth() const
    {
        return mWidth;
    }
    [[nodiscard]] int getHeight() const
    {
        return mHeight;
    }
    [[nodiscard]] float getAspectRatio() const
    {
        return static_cast<float>(mWidth) / mHeight;
    }

    template <typename EventType>
    [[nodiscard]] EventType getEvent() const
    {
        ASSERT(std::holds_alternative<EventType>(mEventData));
        return std::get<EventType>(mEventData);
    }

    static IWindow* createWindow(const uint32_t width = 1280, const uint32_t height = 720);

protected:
    EventData mEventData;
    int mWidth;
    int mHeight;
};