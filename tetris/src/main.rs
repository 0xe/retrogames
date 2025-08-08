use std::error::Error;
mod blocks;
mod board;
mod render;

fn main() -> Result<(), Box<dyn Error>> {
    render::game_loop();
    Ok(())
}