#pragma once

#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace paasphoos
{

/**
 * High-performance, zero-allocation Easing and Motion Engine.
 * Implements non-linear physics curves:
 *  - cubic-bezier(0.16, 1, 0.3, 1) for entrances
 *  - cubic-bezier(0.65, 0, 0.35, 1) for transitions
 *  - easeOutExpo, easeOutCubic, easeInOutCubic
 */
class TweenEngine
{
public:
    static float clamp01(float t)
    {
        return std::max(0.0f, std::min(1.0f, t));
    }

    static float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    // ease-out-expo: y = t == 1 ? 1 : 1 - 2^(-10 * t)
    static float easeOutExpo(float t)
    {
        t = clamp01(t);
        if (t >= 0.999f) return 1.0f;
        return 1.0f - std::pow(2.0f, -10.0f * t);
    }

    // ease-out-cubic: y = 1 - (1 - t)^3
    static float easeOutCubic(float t)
    {
        t = clamp01(t);
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    // ease-in-out-cubic: smooth symmetric ease
    static float easeInOutCubic(float t)
    {
        t = clamp01(t);
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    }

    // cubic-bezier(0.16, 1, 0.3, 1) - Natural entrance curve
    static float easeEntrance(float t)
    {
        t = clamp01(t);
        // High-precision analytic approximation of cubic-bezier(0.16, 1, 0.3, 1)
        // Solved via Newton-Raphson approximation
        return solveCubicBezier(0.16f, 1.0f, 0.3f, 1.0f, t);
    }

    // cubic-bezier(0.65, 0, 0.35, 1) - Smooth transition curve
    static float easeTransition(float t)
    {
        t = clamp01(t);
        return solveCubicBezier(0.65f, 0.0f, 0.35f, 1.0f, t);
    }

    /**
     * Check if the user has requested Reduced Motion in Windows Settings.
     */
    static bool isReducedMotionRequested()
    {
#if JUCE_WINDOWS
        BOOL clientAnimationEnabled = TRUE;
        if (SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &clientAnimationEnabled, 0))
        {
            if (!clientAnimationEnabled)
                return true;
        }
#endif
        return false;
    }

private:
    static float sampleBezierX(float p1x, float p2x, float t)
    {
        // p0 = 0, p3 = 1
        float inv = 1.0f - t;
        return 3.0f * inv * inv * t * p1x + 3.0f * inv * t * t * p2x + t * t * t;
    }

    static float sampleBezierY(float p1y, float p2y, float t)
    {
        float inv = 1.0f - t;
        return 3.0f * inv * inv * t * p1y + 3.0f * inv * t * t * p2y + t * t * t;
    }

    static float sampleBezierDerivativeX(float p1x, float p2x, float t)
    {
        float inv = 1.0f - t;
        return 3.0f * inv * inv * p1x + 6.0f * inv * t * (p2x - p1x) + 3.0f * t * t * (1.0f - p2x);
    }

    static float solveCubicBezier(float p1x, float p1y, float p2x, float p2y, float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;

        // Newton-Raphson root-finding for parameter t matching x
        float t = x;
        for (int i = 0; i < 6; ++i)
        {
            float currentX = sampleBezierX(p1x, p2x, t) - x;
            if (std::abs(currentX) < 1e-4f)
                break;
            float dX = sampleBezierDerivativeX(p1x, p2x, t);
            if (std::abs(dX) < 1e-5f)
                break;
            t -= currentX / dX;
            t = clamp01(t);
        }
        return sampleBezierY(p1y, p2y, t);
    }
};

} // namespace paasphoos
