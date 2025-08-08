use std::error::Error;
use rand::Rng;
use sdl2::pixels::Color;
use crate::blocks;

#[derive(Debug, Clone, Copy)]
pub enum BlockType {
    I, O, T, S, Z, J, L
}

pub const BLOCK_COLORS: [(u8, u8, u8); 7] = [
    (0, 255, 255), // I - Cyan
    (255, 255, 0), // O - Yellow
    (128, 0, 128), // T - Purple
    (0, 255, 0),   // S - Green
    (255, 0, 0),   // Z - Red
    (0, 0, 255),   // J - Blue
    (255, 165, 0), // L - Orange
];

// Each shape: [rotation][block][(x, y)]
pub const BLOCK_SHAPES: [[[(i32, i32); 4]; 4]; 7] = [
    // I
    [
        [(0,1), (1,1), (2,1), (3,1)],
        [(2,0), (2,1), (2,2), (2,3)],
        [(0,2), (1,2), (2,2), (3,2)],
        [(1,0), (1,1), (1,2), (1,3)],
    ],
    // O
    [
        [(1,0), (2,0), (1,1), (2,1)],
        [(1,0), (2,0), (1,1), (2,1)],
        [(1,0), (2,0), (1,1), (2,1)],
        [(1,0), (2,0), (1,1), (2,1)],
    ],
    // T
    [
        [(1,0), (0,1), (1,1), (2,1)],
        [(1,0), (1,1), (2,1), (1,2)],
        [(0,1), (1,1), (2,1), (1,2)],
        [(1,0), (0,1), (1,1), (1,2)],
    ],
    // S
    [
        [(1,0), (2,0), (0,1), (1,1)],
        [(1,0), (1,1), (2,1), (2,2)],
        [(1,1), (2,1), (0,2), (1,2)],
        [(0,0), (0,1), (1,1), (1,2)],
    ],
    // Z
    [
        [(0,0), (1,0), (1,1), (2,1)],
        [(2,0), (1,1), (2,1), (1,2)],
        [(0,1), (1,1), (1,2), (2,2)],
        [(1,0), (0,1), (1,1), (0,2)],
    ],
    // J
    [
        [(0,0), (0,1), (1,1), (2,1)],
        [(1,0), (2,0), (1,1), (1,2)],
        [(0,1), (1,1), (2,1), (2,2)],
        [(1,0), (1,1), (0,2), (1,2)],
    ],
    // L
    [
        [(2,0), (0,1), (1,1), (2,1)],
        [(1,0), (1,1), (1,2), (2,2)],
        [(0,1), (1,1), (2,1), (0,2)],
        [(0,0), (1,0), (1,1), (1,2)],
    ],
];

#[derive(Debug, Clone, Copy)]
pub struct Tetromino {
    pub kind: BlockType,
    pub rotation: usize, // 0-3
    pub x: i32,
    pub y: i32,
}

impl Tetromino {
    pub fn new(kind: BlockType, x: i32, y: i32) -> Self {
        Tetromino { kind, rotation: 0, x, y }
    }
    pub fn cells(&self) -> [(i32, i32); 4] {
        let shape = BLOCK_SHAPES[self.kind as usize][self.rotation];
        [
            (self.x + shape[0].0, self.y + shape[0].1),
            (self.x + shape[1].0, self.y + shape[1].1),
            (self.x + shape[2].0, self.y + shape[2].1),
            (self.x + shape[3].0, self.y + shape[3].1),
        ]
    }
    pub fn rotate(&mut self) {
        self.rotation = (self.rotation + 1) % 4;
    }
    pub fn color(&self) -> (u8, u8, u8) {
        (255, 255, 255)
        // BLOCK_COLORS[self.kind as usize]
    }
}

pub fn random_tetromino(x: i32, y: i32) -> Tetromino {
    let mut rng = rand::thread_rng();
    let kind = match rng.gen_range(0..7) {
        0 => BlockType::I,
        1 => BlockType::O,
        2 => BlockType::T,
        3 => BlockType::S,
        4 => BlockType::Z,
        5 => BlockType::J,
        _ => BlockType::L,
    };
    Tetromino::new(kind, x, y)
}

#[test]
fn test_random_block() {
    let b = random_tetromino(0, 0);
    println!("Here's a random block: {:#?}", b);
    assert!(true);
}