//
// Created by cyber on 21.03.20.
//

#include <stdio.h>
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <RepPanel/custom_themes/lv_theme_rep_panel_light.h>
#include "reppanel.h"
#include "reppanel_request.h"

#define CANCEL_BTN_TXT  "#ffffff Cancel#"
#define DELETE_BTN_TXT  "#c43145 "LV_SYMBOL_TRASH" Delete#"
#define SIM_BTN_TXT     LV_SYMBOL_EYE_OPEN" Simulate"
#define PRINT_BTN_TXT   REP_PANEL_DARK_ACCENT_STR " " LV_SYMBOL_PLAY" Print#"

#define JOBS_ROOT_DIR  "0:/gcodes"
#define JOBS_EMPTY ""
#define BACK_TXT    "Back"
#define MAX_LEN_DIR 64

file_tree_elem_t reprap_jobs[MAX_NUM_JOBS];

lv_obj_t *jobs_container;
lv_obj_t *jobs_list;
lv_obj_t *msg_box1;
lv_obj_t *msg_box2;
lv_obj_t *msg_box3;
lv_obj_t *preloader;

char parent_dir[MAX_LEN_DIR];
reprap_job_t *edit_job;

static void print_file_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_RELEASED) {
        if (strcmp(lv_mbox_get_active_btn_text(msg_box3), "Yes") == 0) {
            printf("Printing %s/%s\n", edit_job->dir, edit_job->name);
            // TODO: Print file
            lv_obj_del_async(msg_box3);
            display_jobstatus();
        } else {
            lv_obj_del_async(msg_box3);
        }
    }
}

static void delete_file_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_RELEASED) {
        if (strcmp(lv_mbox_get_active_btn_text(msg_box2), "Yes") == 0) {
            printf("Deleting %s/%s\n", edit_job->dir, edit_job->name);
            // TODO: Delete file
            lv_obj_del_async(msg_box2);
            lv_obj_del_async(msg_box1);
        } else {
            lv_obj_del_async(msg_box2);
        }
    }
}

static void job_action_handler(lv_obj_t *obj, lv_event_t event) {
    if(event == LV_EVENT_CLICKED) {
        if (strcmp(lv_mbox_get_active_btn_text(msg_box1), CANCEL_BTN_TXT) == 0) {
            printf("Close window. No action\n");
            lv_obj_del_async(msg_box1);
        } else if (strcmp(lv_mbox_get_active_btn_text(msg_box1), DELETE_BTN_TXT) == 0) {
            static const char * btns[] ={"Yes", "No", ""};
            msg_box2 = lv_mbox_create(lv_layer_top(), NULL);
            lv_mbox_set_text(msg_box2, "Do you really want to delete this file?");
            lv_mbox_add_btns(msg_box2, btns);
            lv_obj_set_event_cb(msg_box2, delete_file_handler);
            lv_obj_set_width(msg_box2, lv_disp_get_hor_res(NULL) - 20);
            lv_obj_align(msg_box2, lv_layer_top(), LV_ALIGN_CENTER, 0, 0);
        } else if (strcmp(lv_mbox_get_active_btn_text(msg_box1), PRINT_BTN_TXT) == 0) {
            printf("Print %s/%s\n", edit_job->dir, edit_job->name);
            // TODO: Print file
            lv_obj_del_async(msg_box1);
            display_jobstatus();
        } else if (strcmp(lv_mbox_get_active_btn_text(msg_box1), SIM_BTN_TXT) == 0) {
            printf("Simulate %s/%s\n", edit_job->dir, edit_job->name);
            // TODO: Simulate file
            lv_obj_del_async(msg_box1);
        }
    }
}

static void _job_clicked_event_handler(lv_obj_t *obj, lv_event_t event) {
    int selected_indx = lv_list_get_btn_index(jobs_list, obj);
    // check if back button exists
    if (strcmp(((reprap_job_t*) reprap_jobs[selected_indx].element)->dir, JOBS_ROOT_DIR) != 0) {
        if (selected_indx == 0 && event == LV_EVENT_SHORT_CLICKED) {
            // back button was pressed
            printf("Going back to parent %s\n", parent_dir);
            if (!preloader)
                preloader = lv_preload_create(lv_layer_top(), NULL);
            lv_obj_set_size(preloader, 75, 75);
            lv_obj_align_origo(preloader, lv_layer_top(), LV_ALIGN_CENTER, 0, 0);
            request_jobs_async(parent_dir);
            return;
        } else if (selected_indx != 0) {
            // no back button pressed
            // decrease index to match with reprap_macros array indexing
            selected_indx--;
        }
    }
    edit_job = ((reprap_job_t*) reprap_jobs[selected_indx].element);
    if(event == LV_EVENT_SHORT_CLICKED) {
        if (reprap_jobs[selected_indx].type == TREE_FILE_ELEM) {
            static const char * btns[] ={"Yes", "No", ""};
            msg_box3 = lv_mbox_create(lv_layer_top(), NULL);
            char msg[100];
            sprintf(msg, "Do you want to print %s?", edit_job->name);
            lv_mbox_set_text(msg_box3, msg);
            lv_mbox_add_btns(msg_box3, btns);
            lv_obj_set_event_cb(msg_box3, print_file_handler);
            lv_obj_set_width(msg_box3, lv_disp_get_hor_res(NULL) - 20);
            lv_obj_align(msg_box3, lv_layer_top(), LV_ALIGN_CENTER, 0, 0);
        } else if (reprap_jobs[selected_indx].type == TREE_FOLDER_ELEM) {
            printf("Clicked folder %s (index %i)\n", edit_job->name, selected_indx);
            if (!preloader)
                preloader = lv_preload_create(lv_layer_top(), NULL);
            lv_obj_set_size(preloader, 75, 75);
            lv_obj_align_origo(preloader, lv_layer_top(), LV_ALIGN_CENTER, 0, 0);
            static char tmp_txt_job_path[128];
            sprintf(tmp_txt_job_path, "%s/%s&first=0", edit_job->dir, edit_job->name);
            request_jobs_async(tmp_txt_job_path);
        }
    } else if (event == LV_EVENT_LONG_PRESSED && reprap_jobs[selected_indx].type == TREE_FILE_ELEM) {
        static const char * btns[] ={SIM_BTN_TXT, PRINT_BTN_TXT, DELETE_BTN_TXT, CANCEL_BTN_TXT, ""};
        msg_box1 = lv_mbox_create(lv_layer_top(), NULL);
        lv_mbox_set_text(msg_box1, "Select action");
        lv_mbox_add_btns(msg_box1, btns);
        lv_mbox_set_recolor(msg_box1, true);
        lv_obj_set_width(msg_box1, lv_disp_get_hor_res(NULL) - 30);
        lv_obj_set_event_cb(msg_box1, job_action_handler);
        lv_obj_align(msg_box1, lv_layer_top(), LV_ALIGN_CENTER, 0, 0);
    }
}

void update_job_list_ui(){
    lv_obj_del(preloader);
    if (jobs_list) {
        lv_list_clean(jobs_list);
    } else {
        return;
    }

    // Add back button in case we are not in root directory
    if (strcmp(((reprap_job_t *) reprap_jobs[0].element)->dir, JOBS_ROOT_DIR) != 0) {
        lv_obj_t *back_btn;
        back_btn = lv_list_add_btn(jobs_list, LV_SYMBOL_LEFT, BACK_TXT);
        lv_obj_set_event_cb(back_btn, _job_clicked_event_handler);
        // update parent dir
        strcpy(parent_dir, ((reprap_job_t *) reprap_jobs[0].element)->dir);
        char *pch;
        pch = strrchr(parent_dir, '/');
        parent_dir[pch - parent_dir] = '\0';
    } else {
        strcpy(parent_dir, JOBS_EMPTY);
    }
    for (int i = 0; reprap_jobs[i].element != NULL; i++) {
        lv_obj_t *list_btn;
        if (reprap_jobs[i].type == TREE_FOLDER_ELEM)
            list_btn = lv_list_add_btn(jobs_list, LV_SYMBOL_DIRECTORY, ((reprap_job_t *) reprap_jobs[i].element)->name);
        else
            list_btn = lv_list_add_btn(jobs_list, LV_SYMBOL_FILE, ((reprap_job_t *) reprap_jobs[i].element)->name);
        lv_obj_set_event_cb(list_btn, _job_clicked_event_handler);
    }
}

void draw_jobselect(lv_obj_t *parent_screen) {
    jobs_container = lv_cont_create(parent_screen, NULL);
    lv_cont_set_layout(jobs_container, LV_LAYOUT_COL_M);
    lv_cont_set_fit(jobs_container, LV_FIT_FILL);

    preloader = lv_preload_create(jobs_container, NULL);
    lv_obj_set_size(preloader, 75, 75);

    jobs_list = lv_list_create(jobs_container, NULL);
    lv_obj_set_size(jobs_list, LV_HOR_RES-10, lv_disp_get_ver_res(NULL) - (lv_obj_get_height(cont_header) + 5));

    request_jobs_async("0:/macros&first=0");
}