package com.android.support;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

	// Game activity class name.
	public String GameActivity = "com.unity3d.player.UnityPlayerActivity";
	public boolean hasLaunched = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		// YouTube tutorial click handler
		ImageView img_yttutorial = findViewById(R.id.img_yttutorial);
		img_yttutorial.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View p1) {
				Intent intent = new Intent("android.intent.action.VIEW");
				intent.setData(Uri.parse("https://www.youtube.com/@PuruliaCheatz"));
				startActivity(intent);
			}
		});

		// Handling layouts and code snippets
		LinearLayout[] linearLayoutArr = new LinearLayout[] { findViewById(R.id.layout1), findViewById(R.id.layout2),
				findViewById(R.id.layout3), findViewById(R.id.layout4) };

		for (int i = 0; i < 4; i++) {
			TextView subTitle = linearLayoutArr[i].findViewById(R.id.subtitle);
			TextView headingText = linearLayoutArr[i].findViewById(R.id.textheading);
			final TextView textCode = linearLayoutArr[i].findViewById(R.id.textcode);
			final TextView btnCopy = linearLayoutArr[i].findViewById(R.id.btncopy);

			switch (i) {
			case 0:
				subTitle.setText(
						"1. System Floating Window Permission (Add the below code in AndroidManifest.xml)\n\n[Note: You do not need to add this permission if you use the \"Without Overlay Permission\" code to the MainActivity of the game.]");
				headingText.setText("Floating Window");
				textCode.setText("<uses-permission android:name=\"android.permission.SYSTEM_ALERT_WINDOW\"/>");
				break;
			case 1:
				subTitle.setText(
						"2. Service of Offset Tester Menu (Add the below code above the end of </application> tag in AndroidManifest.xml)");
				headingText.setText("Service");
				textCode.setText(
						"<service\nandroid:name=\"com.android.support.Launcher\"\nandroid:enabled=\"true\"\nandroid:exported=\"false\"\nandroid:stopWithTask=\"true\"/>");
				break;
			case 2:
				subTitle.setText(
						"3. Mainactivity of Game (Search \"onCreate\" methods (Don't use quotes \" \") then paste the below code inside the onCreate method)\n\n• With Overlay Permission");
				headingText.setText("MainActivity");
				textCode.setText("invoke-static {p0}, Lcom/android/support/Main;->Start(Landroid/content/Context;)V");
				break;
			case 3:
				subTitle.setText("• Without Overlay Permission");
				headingText.setText("MainActivity");
				textCode.setText(
						"invoke-static {p0}, Lcom/android/support/Main;->StartWithoutPermission(Landroid/content/Context;)V");
				break;
			default:
				break;
			}

			// Handle copy button click
			btnCopy.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View p1) {
					ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
					ClipData clip = ClipData.newPlainText(null, textCode.getText());
					clipboard.setPrimaryClip(clip);

					btnCopy.setText("✓ Copied!");
					new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
						@Override
						public void run() {
							btnCopy.setText("❏ Copy Code");
						}
					}, 2000);
				}
			});
		}

		// Launch the game activity
		if (!hasLaunched) {
			try {
				hasLaunched = true;
				Intent gameIntent = new Intent(MainActivity.this, Class.forName(GameActivity));
				startActivity(gameIntent);
				Main.Start(this);
				return; // Prevent further execution
			} catch (ClassNotFoundException e) {
				Log.e("Mod_menu", "Error. Game's main activity does not exist");
				// Provide feedback to user if game activity is not found
				Toast.makeText(MainActivity.this, "Error: Game's main activity does not exist", Toast.LENGTH_LONG)
						.show();
			}
		}

		// Fallback to mod menu if the game activity is not found
		Main.StartWithoutPermission(this);
	}
}