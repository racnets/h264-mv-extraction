/*
 * gtk-viewer.c
 *
 * Created on: 22.01.2014
 * Last modified: 20.06.2017
 *
 * Author: racnets
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  //XintY_t
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <cairo.h>

#include <libavformat/avformat.h>

#include "main.h"     //verbose, verpose_printf
#include "extract.h"  //getMBTypeForMB

#include "gtk-viewer.h"

#define DEFTOSTRING(x) #x
#define M_PI  3.14159265358979323846  //pi




GtkWidget* window;

GtkWidget *pause_toggle;
GtkWidget *next_toggle;
GtkWidget *result_toggle;

GtkWidget *result_label;

GtkWidget *intra_colorbutton;
GtkWidget *predicted_colorbutton;
GtkWidget *skip_colorbutton;

GtkWidget *status_bar_1;
GtkWidget *status_bar_2;
GtkWidget *status_bar_3;
GtkWidget *status_bar_4;

cairo_surface_t *avFrame_image = NULL;
int avFrame_request = FALSE;

#define PLOTTER_BUFFER 256
#define PLOTTER_ITEMS  12
struct {
	int value[PLOTTER_ITEMS][PLOTTER_BUFFER];
	int write_pos;
} plot_data;

/* 
 * helper
 */
int viewer_want_avFrame(void) {
	return (avFrame_request == TRUE);
}

static void viewer_draw_image(GtkWidget* widget, cairo_t *cr, cairo_surface_t *image) {
	debug_printf("called\n");

	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	if(image != NULL) {
		debug_printf("image data present\n");
		
		double scale, scale_width, scale_height;
		double origin_x = 0, origin_y = 0;
		int image_height, image_width;
		image_width = cairo_image_surface_get_width(image);
		image_height = cairo_image_surface_get_height(image);
		scale_width =(float)width / image_width;
		scale_height =(float)height / image_height;
		if(scale_width < scale_height) {
			scale = scale_width;
			origin_y = (height/scale-image_height)/2.0;
		} else {
			scale = scale_height;
			origin_x = (width/scale-image_width)/2.0;
		}
		debug_printf("scale %f(%f/%f)", scale, scale_width, scale_height);
		debug_printf("origin %f/%f", origin_x, origin_y);
		debug_printf("size %d, %d", width, height);
		debug_printf("image size %d, %d",image_width, image_height);
		debug_printf("cairo status: %s", cairo_status_to_string(cairo_surface_status(image)));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		cairo_scale(cr, scale, scale);
		cairo_set_source_surface(cr, image, origin_x, origin_y);
		//~ cairo_set_source_surface(cr, image, 0, 0);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_FAST);
  		cairo_paint(cr);
	} else {
		verbose_printf("no image data to show");
		cairo_set_source_rgb(cr, 255, 255, 255); 
		cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

		cairo_set_font_size(cr, 16);
		cairo_move_to(cr, 20, 100);
		cairo_show_text(cr, "no image data");
		cairo_set_source_rgb(cr, 255, 255, 0); 
		cairo_paint(cr);
	}
	//~ cairo_destroy(cr);
}

/*
 * 
 */
static gboolean avFrame_draw_event(GtkWidget* widget, cairo_t *cr, gpointer user_data) {      
	avFrame_request = TRUE;

	viewer_draw_image(widget, cr, avFrame_image);
	
	return TRUE;
}

static void do_plot(cairo_t *cr, cairo_pattern_t *pattern, int index, double scale_w, int height) {
	int max = 0;
	int min = 0;	
	int i;
	// finds extrema
	for (i=0;i<PLOTTER_BUFFER;i++){
		int date = plot_data.value[index][i];
		if (date > max) max = date;
		else if (date < min) min = date;
	}

	// to calculate horizontal scale and zero-line
	double scale_h = (max == min)? 1.0: (double)height / (max - min);
	int neutral = max * scale_h;

	cairo_set_source(cr, pattern);
	cairo_set_line_width(cr,1);
	cairo_save(cr);
	
	// rotate and scale
	cairo_matrix_t matrix;
	cairo_matrix_init(&matrix,
		scale_w, 0,
		0, -scale_h,
		0, neutral
	);
	cairo_transform(cr, &matrix);

	// plot
	for (i=0;i<PLOTTER_BUFFER;i++){
		int pos = plot_data.write_pos - i;
		if (pos < 0) pos += PLOTTER_BUFFER;
		int date = plot_data.value[index][pos];
		cairo_line_to(cr, i, date);
	}
	cairo_restore(cr);
	cairo_stroke(cr);
}

static gboolean plotter_on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {      
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);
	verbose_printf("width/height: %d/%d", width, height);
	double scale_w = (double)width / PLOTTER_BUFFER;

	// white background
	cairo_set_source_rgb(cr, 255, 255, 255); 
	cairo_paint(cr);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(result_toggle))) do_plot(cr, cairo_pattern_create_rgb(0,1,0), 3, scale_w, height);

	return TRUE;
}

/*
 * initialize GUI
 * 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_init(int *argc, char **argv[]) {
	GtkBuilder *builder;
	GObject *win;
	GObject *tmp_obj;
	
	/* initialize gtk */
	gtk_init(argc, argv);
	
	/* Construct a GtkBuilder instance and load UI description */
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "h264-mv-extraction.ui", NULL);
	
	/*  CSS */
	GtkCssProvider *provider = gtk_css_provider_new();
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	const gchar* css_path = "gtk-viewer.css";
	GError *error = 0;	
	gtk_css_provider_load_from_path(GTK_CSS_PROVIDER(provider), css_path, &error);
	if (error) printf("error loading css \"%s\": %s\n", css_path, error->message);
	
	g_object_unref(provider);
	

	/* Connect signal handlers and get adjustments to the constructed widgets. */
	/* main window */
	win = gtk_builder_get_object(builder, "window");
	window = GTK_WIDGET(win);
	g_signal_connect(win, "destroy", G_CALLBACK(gtk_widget_destroyed), &window);
	
	/* result label */
   	tmp_obj = gtk_builder_get_object(builder, "result_label");
	result_label = GTK_WIDGET(tmp_obj);
	
	/* AVFrame drawing area */
	tmp_obj = gtk_builder_get_object(builder, "avFrame_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(avFrame_draw_event), NULL); 	
	
	/* get result pause-button */
	tmp_obj = gtk_builder_get_object(builder, "pause_toggle");
	pause_toggle = GTK_WIDGET(tmp_obj);

	/* get result pause-button */
	tmp_obj = gtk_builder_get_object(builder, "next_toggle");
	next_toggle = GTK_WIDGET(tmp_obj);

	/* get result toggle-button */
	tmp_obj = gtk_builder_get_object(builder, "result_toggle");
	result_toggle = GTK_WIDGET(tmp_obj);

	/* get intra colorbutton */
	tmp_obj = gtk_builder_get_object(builder, "intra_colorbutton");
	intra_colorbutton = GTK_WIDGET(tmp_obj);

	/* get predicted colorbutton */
	tmp_obj = gtk_builder_get_object(builder, "predicted_colorbutton");
	predicted_colorbutton = GTK_WIDGET(tmp_obj);

	/* get skip colorbutton */
	tmp_obj = gtk_builder_get_object(builder, "skip_colorbutton");
	skip_colorbutton = GTK_WIDGET(tmp_obj);

	/* plotter drawing area */
	tmp_obj = gtk_builder_get_object(builder, "plotter_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(plotter_on_draw_event), NULL); 	
	
	/* exit button */
	tmp_obj = gtk_builder_get_object(builder, "exit_button");
	g_signal_connect(tmp_obj, "clicked", G_CALLBACK(gtk_widget_destroyed), &window); 	
	
	/* status bar */
	tmp_obj = gtk_builder_get_object(builder, "status-bar_1");
	status_bar_1 = GTK_WIDGET(tmp_obj);
	tmp_obj = gtk_builder_get_object(builder, "status-bar_2");
	status_bar_2 = GTK_WIDGET(tmp_obj);
	tmp_obj = gtk_builder_get_object(builder, "status-bar_3");
	status_bar_3 = GTK_WIDGET(tmp_obj);
	tmp_obj = gtk_builder_get_object(builder, "status-bar_4");
	status_bar_4 = GTK_WIDGET(tmp_obj);
	
	
	gtk_widget_show_all(window);
	
	return EXIT_SUCCESS;
}

int viewer_is_paused(void) {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(next_toggle))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(next_toggle), FALSE);
		return FALSE;
	}
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pause_toggle));
}

/*
 * update GUI
 * 
 * needs to be called frequently
 * 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_update() {
	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
		
	while(gtk_events_pending())
		gtk_main_iteration();
		
	return EXIT_SUCCESS;
}

void drawMbType(cairo_surface_t *image, AVFrame *frame) {
	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;

	cairo_t *cr = cairo_create(avFrame_image);
	
	cairo_set_source_rgb(cr, 255, 0, 0);
	cairo_paint(cr);
	
	cairo_set_line_width(cr, 1);

	cairo_scale(cr, 16, 16);	

	int x,y;
	//~ double color;
	for (y=0; y< mb_height; y++) {
		for (x=0; x< mb_width; x++) {
			switch (getMBTypeForMB(frame, x, y)) {
				case 'i':
					/* intra 4x4 */
					cairo_set_source_rgb(cr, 255, 255, 255);
					break;
				case 'I':
					/* intra 16x16 */
					cairo_set_source_rgb(cr, 200, 200, 200);
					break;
				case 'p':
				case '-':
				case '|':
				case '+':
					/* p-type mbs */
					cairo_set_source_rgb(cr, 0, 255, 0);
					break;
				case 's':
					/* skip */
					cairo_set_source_rgb(cr, 255, 255, 0);
					break;
				default: 
					/* other */
					cairo_set_source_rgb(cr, 255, 0, 0);
			}
			cairo_rectangle(cr, x, y, 1, 1);
			cairo_fill(cr);
		}
	}

	return;
}

void drawMv(cairo_surface_t *image, AVFrame *frame) {
	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;

	cairo_t *cr = cairo_create(avFrame_image);

	cairo_set_source_rgb(cr, 123, 0, 0);
	cairo_set_line_width(cr, 1);

	int x,y,mb_x,mb_y;
	int mv_x, mv_y;
	for (mb_y=0; mb_y< mb_height; mb_y++) {
		for (mb_x=0; mb_x< mb_width; mb_x++) {
			if (getMVForMB(frame, mb_x, mb_y, &mv_x, &mv_y)) {
				x = (mb_x << 4) + 8;
				y = (mb_y << 4) + 8;
				
				cairo_move_to(cr, x, y);
				cairo_rel_line_to(cr, mv_x, mv_y);
				cairo_stroke(cr);
				cairo_move_to(cr, x, y);
				cairo_arc(cr, x, y, 2, 0, 2 * M_PI);
				cairo_stroke(cr);
			}

		}
	}
}


/*
 * set frame in GUI 
 * 
 * @param *frame: AVFrame
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_set_avFrame(AVFrame *frame, int mvs, double motX, double motY) {
	debug_printf("called with *img: %#x \twidth: %d\theight: %d", (int)frame, frame->width, frame->height);
	
	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
	
	/* if no data is assigned return */
	if (frame == NULL) return EXIT_FAILURE;
	
	/* setup image */
 	if(avFrame_image != NULL) {
		cairo_surface_destroy(avFrame_image);
 	}

	/* reset request flag */
	avFrame_request = FALSE;

	/* create empty image */
	debug_printf("create image of size: width: %d\theight: %d\tstride: %d\n", frame->width, frame->height, frame->linesize[0]);
	avFrame_image = cairo_image_surface_create (CAIRO_FORMAT_RGB24, frame->width, frame->height);

	drawMbType(avFrame_image, frame);
	drawMv(avFrame_image, frame);

	/* result label */
	char _result_label_str[sizeof("/") + 2*strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_result_label_str, "%0.3f/%0.3f", motX, motY);
	gtk_label_set_text(GTK_LABEL(result_label), _result_label_str);
	
	/* status bar */
	/* picture number*/
	char _frame_label_str[sizeof("frame: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_frame_label_str, "frame: %d", frame->coded_picture_number);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_1), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_1), "frame"), _frame_label_str);

	/* picture size */
	char _size_label_str[sizeof("size:  x ") + 2 * strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_size_label_str, "size: %d x %d", frame->width, frame->height);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_2), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_2), "size"), _size_label_str);

	/* picture type */
	char _picture_type_label_str[sizeof("type: X")];
	sprintf(_picture_type_label_str, "type: %c", av_get_picture_type_char(frame->pict_type));
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_3), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_3), "type"), _picture_type_label_str);

	/* motion vectors */
	char _mvs_label_str[sizeof("motion vectors: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_mvs_label_str, "motion vectors: %d", mvs);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_4), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_4), "motion_vectors"), _mvs_label_str);

	/* force redraw */
	gtk_widget_queue_draw(window);
		
	return EXIT_SUCCESS;
}
