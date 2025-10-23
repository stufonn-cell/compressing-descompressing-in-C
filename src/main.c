#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "file_manager.h"

// Global widgets
typedef struct {
    GtkWidget *window;
    GtkWidget *input_label;
    GtkWidget *output_label;
    GtkWidget *status_label;
    GtkWidget *compress_btn;
    GtkWidget *decompress_btn;
    char *input_path;
    char *output_path;
} AppWidgets;

// Callback for selecting input file or directory
static void on_select_input_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Input File or Directory",
                                                     GTK_WINDOW(widgets->window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Select", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    // Allow selecting both files and folders
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
    
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *path;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        path = gtk_file_chooser_get_filename(chooser);
        
        if (widgets->input_path) {
            g_free(widgets->input_path);
        }
        widgets->input_path = path;
        
        // Display path info
        char info[512];
        snprintf(info, sizeof(info), "Input: %s", widgets->input_path);
        gtk_label_set_text(GTK_LABEL(widgets->input_label), info);
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Input selected");
    }
    
    gtk_widget_destroy(dialog);
}

// Callback for selecting output file or directory
static void on_select_output_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Output File or Directory",
                                                     GTK_WINDOW(widgets->window),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Select", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *path;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        path = gtk_file_chooser_get_filename(chooser);
        
        if (widgets->output_path) {
            g_free(widgets->output_path);
        }
        widgets->output_path = path;
        
        char info[512];
        snprintf(info, sizeof(info), "Output: %s", widgets->output_path);
        gtk_label_set_text(GTK_LABEL(widgets->output_label), info);
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Output selected");
    }
    
    gtk_widget_destroy(dialog);
}

// Callback for Compress button
static void on_compress_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->input_path) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Please select an input");
        return;
    }
    
    if (!widgets->output_path) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Please select an output");
        return;
    }
    
    // Use file_manager compress function
    fm_status_t status = fm_compress(widgets->input_path, widgets->output_path);
    
    if (status == FM_STATUS_OK) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Compression completed successfully");
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Status: Compression failed (error code: %d)", status);
        gtk_label_set_text(GTK_LABEL(widgets->status_label), error_msg);
    }
}

// Callback for Decompress button
static void on_decompress_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->input_path) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Please select an input");
        return;
    }
    
    if (!widgets->output_path) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Please select an output");
        return;
    }
    
    // Use file_manager decompress function
    fm_status_t status = fm_decompress(widgets->input_path, widgets->output_path);
    
    if (status == FM_STATUS_OK) {
        gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Decompression completed successfully");
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Status: Decompression failed (error code: %d)", status);
        gtk_label_set_text(GTK_LABEL(widgets->status_label), error_msg);
    }
}

// Callback for Clear button
static void on_clear_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (widgets->input_path) {
        g_free(widgets->input_path);
        widgets->input_path = NULL;
    }
    
    if (widgets->output_path) {
        g_free(widgets->output_path);
        widgets->output_path = NULL;
    }
    
    gtk_label_set_text(GTK_LABEL(widgets->input_label), "Input: No file or directory selected");
    gtk_label_set_text(GTK_LABEL(widgets->output_label), "Output: No file or directory selected");
    gtk_label_set_text(GTK_LABEL(widgets->status_label), "Status: Ready");
}

// Build the UI
static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = g_new(AppWidgets, 1);
    widgets->input_path = NULL;
    widgets->output_path = NULL;
    
    // Create main window
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "File Compression/Decompression Tool");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 600, 400);
    
    // Create main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(widgets->window), vbox);
    
    // Title label
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<big><b>File Compression/Decompression Tool</b></big>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 10);
    
    // Input section
    GtkWidget *input_frame = gtk_frame_new("Input");
    gtk_box_pack_start(GTK_BOX(vbox), input_frame, FALSE, FALSE, 5);
    
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(input_box), 10);
    gtk_container_add(GTK_CONTAINER(input_frame), input_box);
    
    widgets->input_label = gtk_label_new("Input: No file or directory selected");
    gtk_widget_set_halign(widgets->input_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(input_box), widgets->input_label, FALSE, FALSE, 5);
    
    GtkWidget *select_input_btn = gtk_button_new_with_label("Select Input");
    g_signal_connect(select_input_btn, "clicked", G_CALLBACK(on_select_input_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(input_box), select_input_btn, FALSE, FALSE, 0);
    
    // Output section
    GtkWidget *output_frame = gtk_frame_new("Output");
    gtk_box_pack_start(GTK_BOX(vbox), output_frame, FALSE, FALSE, 5);
    
    GtkWidget *output_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(output_box), 10);
    gtk_container_add(GTK_CONTAINER(output_frame), output_box);
    
    widgets->output_label = gtk_label_new("Output: No file or directory selected");
    gtk_widget_set_halign(widgets->output_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(output_box), widgets->output_label, FALSE, FALSE, 5);
    
    GtkWidget *select_output_btn = gtk_button_new_with_label("Select Output");
    g_signal_connect(select_output_btn, "clicked", G_CALLBACK(on_select_output_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(output_box), select_output_btn, FALSE, FALSE, 0);
    
    // Operation buttons
    GtkWidget *operation_frame = gtk_frame_new("Operations");
    gtk_box_pack_start(GTK_BOX(vbox), operation_frame, FALSE, FALSE, 5);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 10);
    gtk_container_add(GTK_CONTAINER(operation_frame), button_box);
    
    widgets->compress_btn = gtk_button_new_with_label("Compress");
    g_signal_connect(widgets->compress_btn, "clicked", G_CALLBACK(on_compress_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(button_box), widgets->compress_btn, TRUE, TRUE, 0);
    
    widgets->decompress_btn = gtk_button_new_with_label("Decompress");
    g_signal_connect(widgets->decompress_btn, "clicked", G_CALLBACK(on_decompress_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(button_box), widgets->decompress_btn, TRUE, TRUE, 0);
    
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(button_box), clear_btn, TRUE, TRUE, 0);
    
    // Status bar
    widgets->status_label = gtk_label_new("Status: Ready");
    gtk_widget_set_halign(widgets->status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), widgets->status_label, FALSE, FALSE, 10);
    
    gtk_widget_show_all(widgets->window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.example.filecompressor", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}