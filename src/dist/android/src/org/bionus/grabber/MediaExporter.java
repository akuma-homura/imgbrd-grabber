package org.bionus.grabber;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

// Called by the Qt JNI bridge. Return 0 on failure, 1 on export, 2 if
// the destination already exists. Do not rely on legacy storage permissions.
public final class MediaExporter {
    private MediaExporter() {}

    public static int exportFile(Context context, String inputPath, String relative) {
        if (context == null || inputPath == null || relative == null) {
            return 0;
        }

        String normalized = relative.replace('\\', '/');
        if (normalized.startsWith("/") || normalized.indexOf('\0') >= 0) {
            return 0;
        }
        String[] components = normalized.split("/");
        if (components.length == 0) {
            return 0;
        }
        for (String component : components) {
            if (component.isEmpty() || component.equals(".") || component.equals("..")
                || component.indexOf(':') >= 0) {
                return 0;
            }
        }
        File source = new File(inputPath);
        if (!source.isFile()) {
            return 0;
        }

        String filename = components[components.length - 1];
        String extension = "";
        int dot = filename.lastIndexOf('.');
        if (dot > 0 && dot < filename.length() - 1) {
            extension = filename.substring(dot + 1).toLowerCase(java.util.Locale.ROOT);
        }
        String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
        if (mime == null) {
            mime = "application/octet-stream";
        }

        StringBuilder directory = new StringBuilder("Pictures/Grabber/");
        for (int i = 0; i < components.length - 1; i++) {
            directory.append(components[i]).append('/');
        }

        // On Android 10+, MediaStore allows user-visible storage without broad
        // filesystem access. Older Android builds use the legacy public folder.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            File publicRoot = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_PICTURES);
            File target = new File(publicRoot, "Grabber/" + normalized);
            File folder = target.getParentFile();
            if (folder == null || (!folder.isDirectory() && !folder.mkdirs())) {
                return 0;
            }
            if (target.exists()) {
                return 2;
            }
            try (InputStream in = new FileInputStream(source);
                 OutputStream out = new FileOutputStream(target)) {
                copy(in, out);
                return 1;
            } catch (Exception error) {
                target.delete();
                return 0;
            }
        }

        final Uri collection;
        if (mime.startsWith("image/")) {
            collection = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
        } else if (mime.startsWith("video/")) {
            collection = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
        } else {
            // Non-image attachments should remain in Downloads; MediaStore
            // won't reliably accept arbitrary ZIP files in Pictures.
            collection = MediaStore.Downloads.EXTERNAL_CONTENT_URI;
            directory = new StringBuilder("Download/Grabber/");
            for (int i = 0; i < components.length - 1; i++) {
                directory.append(components[i]).append('/');
            }
        }

        ContentResolver resolver = context.getContentResolver();
        String relativePath = directory.toString();
        try (Cursor cursor = resolver.query(collection,
                new String[] { MediaStore.MediaColumns._ID },
                MediaStore.MediaColumns.DISPLAY_NAME + "=? AND "
                    + MediaStore.MediaColumns.RELATIVE_PATH + "=?",
                new String[] { filename, relativePath }, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                return 2;
            }
        } catch (Exception error) {
            return 0;
        }

        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, filename);
        values.put(MediaStore.MediaColumns.MIME_TYPE, mime);
        values.put(MediaStore.MediaColumns.RELATIVE_PATH, relativePath);
        values.put(MediaStore.MediaColumns.IS_PENDING, 1);
        Uri destination = null;

        try {
            destination = resolver.insert(collection, values);
            if (destination == null) {
                return 0;
            }
            try (InputStream in = new FileInputStream(source);
                 OutputStream out = resolver.openOutputStream(destination)) {
                if (out == null) {
                    resolver.delete(destination, null, null);
                    return 0;
                }
                copy(in, out);
            }
            ContentValues complete = new ContentValues();
            complete.put(MediaStore.MediaColumns.IS_PENDING, 0);
            resolver.update(destination, complete, null, null);
            return 1;
        } catch (Exception error) {
            if (destination != null) {
                try {
                    resolver.delete(destination, null, null);
                } catch (Exception ignored) {}
            }
            return 0;
        }
    }

    private static void copy(InputStream in, OutputStream out) throws java.io.IOException {
        byte[] buffer = new byte[65536];
        int count;
        while ((count = in.read(buffer)) != -1) {
            out.write(buffer, 0, count);
        }
        out.flush();
    }
}
