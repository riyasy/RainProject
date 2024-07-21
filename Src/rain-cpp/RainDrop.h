#include <windows.h>
#include <d2d1.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

#pragma comment(lib, "d2d1")


// Enum for RainDrop types
enum class RainDropType {
    MainDrop,
    Splatter
};

// RainDrop Class
class RainDrop {
public:
    RainDrop(float windowWidth, float windowHeight, RainDropType type)
        : windowWidth(windowWidth), windowHeight(windowHeight - 20), type(type) {
        Initialize();
    }

    bool DidDropLand() {
        return landedDrop;
    }

    bool ShouldBeErasedAndDeleted() const
    {
        return framesForSplatter > 60;
    }

    void MoveToNewPosition() {
        if (landedDrop)
        {
            return;
        }

        // Update the position of the raindrop
        ellipse.point.x += velocityX;
        ellipse.point.y += velocityY;


        // Apply gravity
        if (type == RainDropType::Splatter)
        {
            velocityY += gravity;
        }


        if (type == RainDropType::MainDrop)
        {
            if (ellipse.point.y + ellipse.radiusY >= windowHeight)
            {
                landedDrop = true;
                for (int i = 0; i < 5; i++)
                {
                    RainDrop* splatter = new RainDrop(windowWidth, windowHeight, RainDropType::Splatter);
                    splatter->UpdatePositionAndSpeed(ellipse.point.x, ellipse.point.y, 2 * (i + 1), velocityY/1.5);
                    splatters.push_back(splatter);
                }
            }
        }

        // Bounce off the edges for Splatter type
        if (type == RainDropType::Splatter) {
            if (ellipse.point.x + ellipse.radiusX > windowWidth || ellipse.point.x - ellipse.radiusX < 0) {
                velocityX = -velocityX;
            }

            if (ellipse.point.y + ellipse.radiusY > windowHeight) {
                ellipse.point.y = windowHeight - ellipse.radiusY; // Keep the ellipse within bounds
                velocityY = -velocityY * bounceDamping; // Bounce with damping
            }

            if (ellipse.point.y - ellipse.radiusY < 0) {
                ellipse.point.y = ellipse.radiusY; // Keep the ellipse within bounds
                velocityY = -velocityY; // Reverse the direction if it hits the top edge
            }
        }
    }

    void UpdatePositionAndSpeed(FLOAT x, FLOAT y, float xSpeed, float ySpeed)
    {
        ellipse.point.x = x;
        ellipse.point.y = y;
        velocityX = xSpeed;
        velocityY = ySpeed;
    }

    void Draw(ID2D1HwndRenderTarget* pRenderTarget, ID2D1SolidColorBrush* pBrush) {
        if (!landedDrop)
        {
            pRenderTarget->FillEllipse(ellipse, pBrush);
        }
        if (splatters.size() > 0)
        {
            framesForSplatter++;
        }
        for (auto drop : splatters) {
            drop->Draw(pRenderTarget, pBrush);
            drop->MoveToNewPosition();
        }
    }

    ~RainDrop()
    {
        for (auto drop : splatters) {
            delete drop;
        }
    }

private:
    D2D1_ELLIPSE ellipse;
    float velocityX;
    float velocityY;
    float gravity;
    float bounceDamping;
    float windowWidth;
    float windowHeight;
    RainDropType type;
    bool landedDrop = false;
    int framesForSplatter = 0;

    std::vector<RainDrop*> splatters;

    void Initialize() {
        // Initialize position and size
        ellipse.point.x = static_cast<float>(rand() % static_cast<int>(windowWidth));
        ellipse.point.y = static_cast<float>((rand() % static_cast<int>(-windowHeight)) / 10 * 10);
        //ellipse.point.x = 0;
        //ellipse.point.y = 0;


        if (type == RainDropType::MainDrop) {
            ellipse.radiusX = 2;
            ellipse.radiusY = 2;
        }
        else {
            ellipse.radiusX = 1.0f;
            ellipse.radiusY = 1.0f;
        }

        // Initialize velocity and physics parameters
        velocityX = 3.0f;
        velocityY = 10.0f;
        gravity = 0.5f;
        bounceDamping = 0.7f;
    }
};