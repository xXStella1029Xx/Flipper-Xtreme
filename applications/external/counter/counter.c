// インクルード 使用メソッドのインポート
#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <counter_icons.h>

// 変数定義
#define MAX_COUNT 99 // カウントの最大数を99に設定
#define BOXTIME 2 // ボタンが押されてから描写されるまでの時間
#define BOXWIDTH 30 // ボックスの横のサイズ(縦にも使われている)
#define MIDDLE_X 64 - BOXWIDTH / 2 // ボックスの横の位置
#define MIDDLE_Y 32 - BOXWIDTH / 2 // ボックスの縦の位置
#define OFFSET_Y 9 // 縦の位置のオフセット

// 情報保存関数
typedef struct {
    FuriMessageQueue* input_queue; // FuriMessageQueueをinput_queueとして定義
    ViewPort* view_port; // ViewPortをview_portとして定義
    Gui* gui; // Guiをguiとして定義
    FuriMutex** mutex; // FuriMutexをmutexとして定義

    int count; // カウントを保存するための変数
    bool pressed; // ボタンが押されているかどうかの変数
    int boxtimer; // 更新間隔を保存するための変数
} Counter;

// アプリ終了時の処理
void state_free(Counter* c) {
    gui_remove_view_port(c->gui, c->view_port); // guiの削除
    furi_record_close(RECORD_GUI); // レコードを終了
    view_port_free(c->view_port); // ビューポートを削除
    furi_message_queue_free(c->input_queue); // メッセージキューを削除
    furi_mutex_free(c->mutex); // mutexを削除
    free(c); // アプリの終了
}

// 入力イベントの処理
static void input_callback(InputEvent* input_event, void* ctx) { // インプットイベントの応答処理
    Counter* c = ctx; // Counter* cにctxをセット
    if(input_event->type == InputTypeShort) { // ボタンが短く押されたら
        furi_message_queue_put(
            c->input_queue, input_event, 0); // (インスタンス, メッセージ, タイムアウト)
    }
}

// 描写の処理
static void render_callback(Canvas* canvas, void* ctx) { // 描写の応答処理
    Counter* c = ctx; // Counter* c に ctxをセット
    furi_check(
        furi_mutex_acquire(c->mutex, FuriWaitForever) ==
        FuriStatusOk); // 実行中の処理がないことを確認
    canvas_clear(canvas); // キャンバスを消す
    canvas_set_color(canvas, ColorBlack); // キャンバスの色を黒にセット
    canvas_set_font(canvas, FontPrimary); // キャンバスのフォントを設定
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignCenter, "Counter :)"); // 文字を表示
    canvas_set_font(canvas, FontBigNumbers); // キャンバスのフォントを設定

    char scount[5]; // 4文字の文字列を格納
    if(c->pressed == true ||
       c->boxtimer > 0) { // ボタンが押されているもしくはboxtimerが0より大きい場合
        canvas_draw_rframe(
            canvas,
            MIDDLE_X,
            MIDDLE_Y + OFFSET_Y,
            BOXWIDTH,
            BOXWIDTH,
            5); // キャンバスに指定した条件のものを描写
        canvas_draw_rframe(
            canvas, MIDDLE_X - 1, MIDDLE_Y + OFFSET_Y - 1, BOXWIDTH + 2, BOXWIDTH + 2, 5);
        canvas_draw_rframe(
            canvas, MIDDLE_X - 2, MIDDLE_Y + OFFSET_Y - 2, BOXWIDTH + 4, BOXWIDTH + 4, 5);
        c->pressed = false; // ボタンを押されていないことにする
        c->boxtimer--; // boxtimerを減らす
    } else {
        canvas_draw_rframe(canvas, MIDDLE_X, MIDDLE_Y + OFFSET_Y, BOXWIDTH, BOXWIDTH, 5);
    }
    snprintf(
        scount,
        sizeof(scount),
        "%d",
        c->count); // 書き込まれた文字数を返す scount : 出力先のポインタ sizeof(scount) : scountの最大バイト数 %d : フォーマット c->count : scountにcountを書き込む
    canvas_draw_str_aligned(
        canvas,
        64,
        32 + OFFSET_Y,
        AlignCenter,
        AlignCenter,
        scount); // ボックスの真ん中にカウントを表示
    furi_mutex_release(c->mutex); // mutexをクリア
}

// アプリの初期化処理
Counter* state_init() {
    Counter* c = malloc(sizeof(Counter));
    c->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    c->view_port = view_port_alloc();
    c->gui = furi_record_open(RECORD_GUI);
    c->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    c->count = 0;
    c->boxtimer = 0;
    view_port_input_callback_set(c->view_port, input_callback, c);
    view_port_draw_callback_set(c->view_port, render_callback, c);
    gui_add_view_port(c->gui, c->view_port, GuiLayerFullscreen);
    return c;
}

// アプリのメイン関数
int32_t counterapp(void) {
    Counter* c = state_init();

    while(1) {
        InputEvent input;
        while(furi_message_queue_get(c->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            furi_check(furi_mutex_acquire(c->mutex, FuriWaitForever) == FuriStatusOk);

            if(input.key == InputKeyBack) {
                furi_mutex_release(c->mutex);
                state_free(c);
                return 0;
            } else if((input.key == InputKeyUp || input.key == InputKeyOk) && c->count < MAX_COUNT) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count++;
            } else if(input.key == InputKeyDown && c->count != 0) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count--;
            }
            furi_mutex_release(c->mutex);
            view_port_update(c->view_port);
        }
    }
    state_free(c);
    return 0;
}
