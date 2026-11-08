package org.bionus.grabber;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import androidx.core.app.NotificationCompat;

public class NotificationUtils
{
    private static final String CHANNEL_ID = "org.bionus.grabber";

    public static void notify(Context context, String title, String message)
    {
        NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (manager == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Grabber", NotificationManager.IMPORTANCE_DEFAULT);
            manager.createNotificationChannel(channel);
        }

        // NotificationCompat supplies the non-deprecated API on both old and
        // new Android versions, including notification channel support.
        Bitmap icon = BitmapFactory.decodeResource(context.getResources(), R.drawable.icon);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(context, CHANNEL_ID);
        builder.setSmallIcon(R.drawable.icon)
            .setLargeIcon(icon)
            .setContentTitle(title)
            .setContentText(message)
            .setDefaults(Notification.DEFAULT_SOUND)
            .setColor(Color.GREEN)
            .setAutoCancel(true);

        manager.notify(0, builder.build());
    }
}
