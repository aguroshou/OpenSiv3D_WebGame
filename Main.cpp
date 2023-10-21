//# include <Siv3D.hpp>
//
//void Main()
//{
//    // Set background color to sky blue
//    Scene::SetBackground(ColorF{ 0.8, 0.9, 1.0 });
//
//    // Create a new font
//    const Font font{ 60 };
//    
//    // Create a new emoji font
//    const Font emojiFont{ 60, Typeface::ColorEmoji };
//    
//    // Set emojiFont as a fallback
//    font.addFallback(emojiFont);
//
//    // Create a texture from an image file
//    const Texture texture{ U"example/windmill.png" };
//
//    // Create a texture from an emoji
//    const Texture emoji{ U"🐈"_emoji };
//
//    // Coordinates of the emoji
//    Vec2 emojiPos{ 300, 150 };
//
//    // Print a text
//    Print << U"Push [A] key";
//
//    while (System::Update())
//    { 
//        // Draw a texture
//        texture.draw(200, 200);
//
//        // Put a text in the middle of the screen
//        font(U"Hello, Siv3D!🚀").drawAt(Scene::Center(), Palette::Black);
//
//        // Draw a texture with animated size
//        emoji.resized(100 + Periodic::Sine0_1(1s) * 20).drawAt(emojiPos);
//
//        // Draw a red transparent circle that follows the mouse cursor
//        Circle{ Cursor::Pos(), 40 }.draw(ColorF{ 1, 0, 0, 0.5 });
//
//        // When [A] key is down
//        if (KeyA.down())
//        {
//            // Print a randomly selected text
//            Print << Sample({ U"Hello!", U"こんにちは", U"你好", U"안녕하세요?" });
//        }
//
//        // When [Button] is pushed
//        if (SimpleGUI::Button(U"Button", Vec2{ 640, 40 }))
//        {
//            // Move the coordinates to a random position in the screen
//            emojiPos = RandomVec2(Scene::Rect());
//        }
//    }
//}


# include <Siv3D.hpp>

void Main()
{
	// ウィンドウを 1280x720 にリサイズする
	Window::Resize(1280, 720);

	// 背景色を設定する
	Scene::SetBackground(ColorF{ 0.4, 0.7, 1.0 });

	// 2D 物理演算のシミュレーションステップ（秒）
	constexpr double StepTime = (1.0 / 200.0);

	// 2D 物理演算のシミュレーション蓄積時間（秒）
	double accumulatedTime = 0.0;

	// 2D 物理演算のワールド
	P2World world;

	// [_] 地面
	const P2Body ground = world.createLine(P2Static, Vec2{ 0, 0 }, Line{ -1600, 0, 1600, 0 });

	// [■] 箱 (Sleep させておく)
	Array<P2Body> boxes;
	{
		for (auto y : Range(0, 12))
		{
			for (auto x : Range(0, 20))
			{
				boxes << world.createRect(P2Dynamic, Vec2{ x * 50, -50 - y * 100 },
					SizeF{ 50, 100 }, P2Material{ .density = 0.02, .restitution = 0.0, .friction = 1.0 })
					.setAwake(false);
			}
		}
	}

	// 振り子の軸の座標
	constexpr Vec2 PivotPos{ 0, -2400 };

	// チェーンを構成するリンク 1 つの長さ
	constexpr double LinkLength = 100.0;

	// チェーンを構成するリンクの数
	constexpr int32 LinkCount = 16;

	// チェーンの長さ
	constexpr double ChainLength = (LinkLength * LinkCount);

	// 鉄球の半径
	constexpr double BallRadius = 200;

	// 鉄球の初期座標
	constexpr Vec2 BallCenter = PivotPos.movedBy(-ChainLength - BallRadius, 0);

	// [●] 鉄球
	const P2Body ball = world.createCircle(P2BodyType::Dynamic, BallCenter, BallRadius,
		P2Material{ .density = 0.5, .restitution = 0.0, .friction = 1.0 });

	// [ ] 振り子の軸（実体がないプレースホルダー）
	const P2Body pivot = world.createPlaceholder(P2BodyType::Static, PivotPos);

	// [-] チェーンを構成するリンク
	Array<P2Body> links;

	// リンクどうしやリンクと鉄球をつなぐジョイント
	Array<P2PivotJoint> joints;
	{
		for (auto i : step(LinkCount))
		{
			// リンクの長方形（隣接するリンクと重なるよう少し大きめに）
			const RectF rect{ Arg::rightCenter = PivotPos.movedBy(i * -LinkLength, 0), LinkLength * 1.2, 20 };

			// categoryBits を 0 にすることで、箱など他の物体と干渉しないようにする
			links << world.createRect(P2Dynamic, rect.center(), rect.size,
				P2Material{ .density = 0.1, .restitution = 0.0, .friction = 1.0 }, P2Filter{ .categoryBits = 0 });

			if (i == 0)
			{
				// 振り子の軸と最初のリンクをつなぐジョイント
				joints << world.createPivotJoint(pivot, links.back(), rect.rightCenter().movedBy(-LinkLength * 0.1, 0));
			}
			else
			{
				// 新しいリンクと、一つ前のリンクをつなぐジョイント
				joints << world.createPivotJoint(links[links.size() - 2], links.back(), rect.rightCenter().movedBy(-LinkLength * 0.1, 0));
			}
		}

		// 最後のリンクと鉄球をつなぐジョイント
		joints << world.createPivotJoint(links.back(), ball, PivotPos.movedBy(-ChainLength, 0));
	}

	// [/] ストッパー
	P2Body stopper = world.createLine(P2Static, BallCenter.movedBy(0, 200), Line{ -400, 200, 400, 0 });

	// 2D カメラ
	Camera2D camera{ Vec2{ 0, -1200 }, 0.25 };

	while (System::Update())
	{
		for (accumulatedTime += Scene::DeltaTime(); StepTime <= accumulatedTime; accumulatedTime -= StepTime)
		{
			// 2D 物理演算のワールドを更新する
			world.update(StepTime);

			// 落下した box は削除する
			boxes.remove_if([](const P2Body& body) { return (2000 < body.getPos().y); });
		}

		// 2D カメラを更新する
		camera.update();
		{
			// 2D カメラから Transformer2D を作成
			const auto t = camera.createTransformer();

			// 地面を描く
			ground.draw(ColorF{ 0.0, 0.5, 0.0 });

			// チェーンを描く
			for (const auto& link : links)
			{
				link.draw(ColorF{ 0.25 });
			}

			// 箱を描く
			for (const auto& box : boxes)
			{
				box.draw(ColorF{ 0.6, 0.4, 0.2 });
			}

			// ストッパーを描く
			stopper.draw(ColorF{ 0.25 });

			// 鉄球を描く
			ball.draw(ColorF{ 0.25 });
		}

		// ストッパーを無くす
		if (stopper && SimpleGUI::Button(U"Go", Vec2{ 1100, 20 }))
		{
			// ストッパーを破棄する
			stopper.release();
		}

		// 2D カメラの操作を描画
		camera.draw(Palette::Orange);
	}
}
