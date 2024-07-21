using Microsoft.Graphics.Canvas;
using Microsoft.Graphics.Canvas.Effects;
using Microsoft.Graphics.Canvas.UI.Xaml;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using System;
using System.Collections.Generic;
using System.Numerics;
using Vanara.PInvoke;
using Windows.Foundation;
using Windows.UI.WebUI;

namespace let_it_rain;

internal class Win2dCanvasController
{
    private readonly CanvasDevice _device = CanvasDevice.GetSharedDevice();
    private readonly Grid _mainLayout;
    private readonly CanvasControl _d2dCanvas;

    private List<RainDrop> _rainDrops = new List<RainDrop>();

    private int i = 100;

    public Win2dCanvasController(Grid mainLayout, CanvasControl d2dCanvas)
    {
        _d2dCanvas = d2dCanvas;
        _mainLayout = mainLayout;
        _d2dCanvas.Draw += CanvasControl_OnDraw;
    }

    private void CanvasControl_OnDraw(CanvasControl sender, CanvasDrawEventArgs args)
    {
        CheckAndGenerateRainDrops();

        foreach (var drop in _rainDrops)
        {
            if (drop.y > 0)
            {
                args.DrawingSession.DrawEllipse(drop.x, drop.y, 1, 1, Colors.Gray, 1);
            }

        }

        //CanvasCommandList cl = new CanvasCommandList(sender);
        //using (CanvasDrawingSession clds = cl.CreateDrawingSession())
        //{
        //    foreach (var drop in _rainDrops)
        //    {
        //        if (drop.y > 0)
        //        {
        //            args.DrawingSession.DrawEllipse(drop.x, drop.y, 1, 1, Colors.Gray, 1);
        //        }

        //    }
        //}
        //GaussianBlurEffect blur = new GaussianBlurEffect();
        //blur.Source = cl;
        //blur.BlurAmount = 5.0f;
        //args.DrawingSession.DrawImage(blur);

        _d2dCanvas.Invalidate();

    }

    private void CheckAndGenerateRainDrops()
    {
        _rainDrops.ForEach(r=>r.CalculateNextPoint());
        _rainDrops.RemoveAll(r => r.y >= 1080);
        var noOfDropsToGenerate = 50 - _rainDrops.Count;
        for (int i = 0; i < noOfDropsToGenerate; i++)
        {
            _rainDrops.Add(new RainDrop());
        }
    }
}

internal class RainDrop
{
    static Random rnd = new Random();
    public int angle;
    public int speed = 10;
    public int x;
    public int y;    

    public RainDrop()
    {
        InitializePosition();
    }

    public void CalculateNextPoint()
    {
        x = x + 2;
        y = y + 10;
    }

    public void InitializePosition()
    {
        x = rnd.Next(0, 1920);
        y = (rnd.Next(-1080, 0) / 10)*10;
    }
}

