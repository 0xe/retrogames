extern crate sdl2;

use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;
use sdl2::sys::{rand, random, UINT8_MAX};
use crate::board::Board;
use crate::blocks::{self, Tetromino, BlockType};

const SLEEP_DURATION: u32 = 1_000_000_000u32;
const WINDOW_WIDTH: u32 = 320;
const WINDOW_HEIGHT: u32 = 640;
const BOARD_COLS: usize = 10;
const BOARD_ROWS: usize = 20;
const CELL_SIZE: u32 = 32;
const DROP_FRAMES: u32 = 30;

pub fn game_loop() {
    let sdl_context = sdl2::init().unwrap();
    let video_subsystem = sdl_context.video().unwrap();

    let window = video_subsystem.window("tetris", WINDOW_WIDTH, WINDOW_HEIGHT).position_centered().build().unwrap();

    let mut canvas = window.into_canvas().build().unwrap();
    canvas.clear();
    canvas.present();
    let mut board = Board::new(BOARD_ROWS as u32, BOARD_COLS as u32);
    let mut current = blocks::random_tetromino((BOARD_COLS / 2 - 2) as i32, 0);
    let mut drop_timer = 0u32;
    let mut event_pump = sdl_context.event_pump().unwrap();
    let mut score = 0;
    let mut game_over = false;
    let mut color = Color::RGB(0, 0, 0);

    'running: loop {
        drop_timer += 1;
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} |
                Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    break 'running // quit
                },
                Event::KeyDown { keycode: Some(Keycode::Left), .. } => {
                    if can_move(&board, &current, -1, 0) {
                        current.x -= 1;
                    }
                },
                Event::KeyDown { keycode: Some(Keycode::Right), .. } => {
                    if can_move(&board, &current, 1, 0) {
                        current.x += 1;
                    }
                },
                Event::KeyDown { keycode: Some(Keycode::Down), .. } => {
                    if can_move(&board, &current, 0, 1) {
                        current.y += 1;
                    }
                },
                Event::KeyDown { keycode: Some(Keycode::Up), .. } => {
                    let mut rotated = current;
                    rotated.rotate();
                    if can_move(&board, &rotated, 0, 0) {
                        current = rotated;
                    }
                },
                _ => {}
            }
        }
        // Gravity
        if drop_timer % DROP_FRAMES == 0 {
            if can_move(&board, &current, 0, 1) {
                current.y += 1;
            } else {
                // Lock piece
                for (x, y) in current.cells() {
                    if y >= 0 && x >= 0 && (y as usize) < board.rows && (x as usize) < board.cols {
                        board.set_cell(y as usize, x as usize, Some(current.kind as u8));
                    } else if y < 0 {
                        game_over = true;
                    }
                }
                score += board.clear_full_lines() * 10;
                // Spawn new
                current = blocks::random_tetromino((BOARD_COLS / 2 - 2) as i32, 0);
                if !can_move(&board, &current, 0, 0) {
                    game_over = true;
                }
            }
        }
        // Draw background with a moving color
        color = update_color(color);
        canvas.set_draw_color(color);
        canvas.clear();
        // Draw board
        for row in 0..board.rows {
            for col in 0..board.cols {
                if let Some(kind) = board.get_cell(row, col) {
                    let (r, g, b) = (255, 255, 255);
                    canvas.set_draw_color(Color::RGB(r, g, b));
                    let _ = canvas.fill_rect(sdl2::rect::Rect::new(
                        (col as i32) * CELL_SIZE as i32,
                        (row as i32) * CELL_SIZE as i32,
                        CELL_SIZE, CELL_SIZE));
                }
            }
        }
        // Draw current piece
        for (x, y) in current.cells() {
            if y >= 0 && x >= 0 && (y as usize) < board.rows && (x as usize) < board.cols {
                let (r, g, b) = current.color();
                canvas.set_draw_color(Color::RGB(r, g, b));
                let _ = canvas.fill_rect(sdl2::rect::Rect::new(
                    (x as i32) * CELL_SIZE as i32,
                    (y as i32) * CELL_SIZE as i32,
                    CELL_SIZE, CELL_SIZE));
            }
        }
        // Draw game over
        if game_over {
            // Draw semi-transparent overlay
            canvas.set_draw_color(Color::RGBA(0, 0, 0, 180));
            let _ = canvas.fill_rect(sdl2::rect::Rect::new(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
            
            // Draw "GAME OVER" using rectangles
            canvas.set_draw_color(Color::RGB(255, 0, 0));
            draw_game_over_text(&mut canvas);
            
            // Draw score
            canvas.set_draw_color(Color::RGB(255, 255, 255));
            draw_score(&mut canvas, score);
            
            canvas.present();
            
            // Wait for key press to exit
            'game_over: loop {
                for event in event_pump.poll_iter() {
                    match event {
                        Event::Quit {..} | 
                        Event::KeyDown { .. } => {
                            break 'game_over;
                        }
                        _ => {}
                    }
                }
                ::std::thread::sleep(Duration::new(0, SLEEP_DURATION / 60));
            }
            break 'running;
        }
        canvas.present();
        ::std::thread::sleep(Duration::new(0, SLEEP_DURATION / 60));
    }
}

fn update_color(c: Color) -> Color {
    // can make the background more dynamic here
    return Color::RGBA(0, 0, 0, 0);
}

fn can_move(board: &Board, tet: &Tetromino, dx: i32, dy: i32) -> bool {
    // check if we can move a piece without it colliding with another
    for (x, y) in tet.cells() {
        let nx = x + dx;
        let ny = y + dy;
        if nx < 0 || nx >= board.cols as i32 || ny >= board.rows as i32 {
            return false;
        }
        if ny >= 0 && nx >= 0 && (ny as usize) < board.rows && (nx as usize) < board.cols {
            if board.get_cell(ny as usize, nx as usize).is_some() {
                return false;
            }
        }
    }
    true
}

fn draw_game_over_text(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>) {
    // Simple pixel art "GAME OVER" text
    let start_x = 60;
    let start_y = 200;
    let pixel_size = 8;
    
    // G
    draw_letter_g(canvas, start_x, start_y, pixel_size);
    // A
    draw_letter_a(canvas, start_x + 25 + 5, start_y, pixel_size);
    // M
    draw_letter_m(canvas, start_x + 50 + 5, start_y, pixel_size);
    // E
    draw_letter_e(canvas, start_x + 75 + 5, start_y, pixel_size);
    
    // O
    draw_letter_o(canvas, start_x + 20 + 5, start_y + 40, pixel_size);
    // V
    draw_letter_v(canvas, start_x + 45 + 5, start_y + 40, pixel_size);
    // E
    draw_letter_e(canvas, start_x + 70 + 5, start_y + 40, pixel_size);
    // R
    draw_letter_r(canvas, start_x + 95 + 5, start_y + 40, pixel_size);
}

fn draw_score(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, score: usize) {
    let start_x = 80;
    let start_y = 350;
    let pixel_size = 6;
    
    // Draw "SCORE:" label
    draw_letter_s(canvas, start_x, start_y, pixel_size);
    draw_letter_c(canvas, start_x + 20, start_y, pixel_size);
    draw_letter_o(canvas, start_x + 40, start_y, pixel_size);
    draw_letter_r(canvas, start_x + 60, start_y, pixel_size);
    draw_letter_e(canvas, start_x + 80, start_y, pixel_size);
    
    // Draw score number
    let score_str = format!("{}", score);
    for (i, digit) in score_str.chars().enumerate() {
        if let Some(d) = digit.to_digit(10) {
            draw_digit(canvas, start_x + 120 + (i as i32 * 15), start_y, pixel_size, d as usize);
        }
    }
}

fn draw_pixel_rect(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    let _ = canvas.fill_rect(sdl2::rect::Rect::new(x, y, size as u32, size as u32));
}

fn draw_letter_g(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    // Simple G pattern
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, 0..=2) | (4, 0..=2) | (2, 1..=3) | (1..=3, 0) | (3, 3) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_a(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, 1..=2) | (1..=4, 0) | (1..=4, 3) | (2, 1..=2) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_m(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0..=4, 0) | (0..=4, 3) | (1, 1..=2) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_e(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, _) | (2, 0..=2) | (4, _) | (1..=3, 0) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_o(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, 1..=2) | (4, 1..=2) | (1..=3, 0) | (1..=3, 3) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_v(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0..=2, 0) | (0..=2, 3) | (3, 1) | (4, 1..=2) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_r(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0..=4, 0) | (0, 1..=2) | (1, 3) | (2, 1..=2) | (3..=4, 3) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_s(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, _) | (2, _) | (4, _) | (1, 0) | (3, 3) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_letter_c(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32) {
    for row in 0..5 {
        for col in 0..4 {
            let should_draw = match (row, col) {
                (0, 1..=2) | (4, 1..=2) | (1..=3, 0) => true,
                _ => false,
            };
            if should_draw {
                draw_pixel_rect(canvas, x + col * size, y + row * size, size);
            }
        }
    }
}

fn draw_digit(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, x: i32, y: i32, size: i32, digit: usize) {
    let patterns = [
        // 0
        vec![vec![1,2], vec![0,3], vec![0,3], vec![0,3], vec![1,2]],
        // 1
        vec![vec![1], vec![1], vec![1], vec![1], vec![0,1,2]],
        // 2
        vec![vec![0,1,2], vec![3], vec![1,2], vec![0], vec![0,1,2,3]],
        // 3
        vec![vec![0,1,2], vec![3], vec![1,2], vec![3], vec![0,1,2]],
        // 4
        vec![vec![0,2], vec![0,2], vec![0,1,2,3], vec![2], vec![2]],
        // 5
        vec![vec![0,1,2,3], vec![0], vec![0,1,2], vec![3], vec![0,1,2]],
        // 6
        vec![vec![1,2], vec![0], vec![0,1,2], vec![0,3], vec![1,2]],
        // 7
        vec![vec![0,1,2,3], vec![3], vec![2], vec![1], vec![1]],
        // 8
        vec![vec![1,2], vec![0,3], vec![1,2], vec![0,3], vec![1,2]],
        // 9
        vec![vec![1,2], vec![0,3], vec![1,2,3], vec![3], vec![1,2]],
    ];
    
    if digit < patterns.len() {
        for (row, cols) in patterns[digit].iter().enumerate() {
            for &col in cols {
                draw_pixel_rect(canvas, x + col * size, y + (row as i32) * size, size);
            }
        }
    }
}
