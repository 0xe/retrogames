use crate::board;

#[derive(Debug, Clone)]
pub struct Board {
    pub rows: usize,
    pub cols: usize,
    pub grid: Vec<Vec<Option<u8>>> // Option<u8>: None = empty, Some(n) = filled with color/index
}

impl Board {
    pub fn new(rows: u32, cols: u32) -> Board {
        Board {
            rows: rows as usize,
            cols: cols as usize,
            grid: vec![vec![None; cols as usize]; rows as usize],
        }
    }

    pub fn get_cell(&self, row: usize, col: usize) -> Option<u8> {
        if row < self.rows && col < self.cols {
            self.grid[row][col]
        } else {
            None
        }
    }

    pub fn set_cell(&mut self, row: usize, col: usize, value: Option<u8>) {
        if row < self.rows && col < self.cols {
            self.grid[row][col] = value;
        }
    }

    pub fn clear_full_lines(&mut self) -> usize {
        let mut new_grid = Vec::with_capacity(self.rows);
        let mut cleared = 0;
        for row in self.grid.iter() {
            if row.iter().all(|cell| cell.is_some()) {
                cleared += 1;
            } else {
                new_grid.push(row.clone());
            }
        }
        while new_grid.len() < self.rows {
            new_grid.insert(0, vec![None; self.cols]);
        }
        self.grid = new_grid;
        cleared
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_board() {
        let mut board = Board::new(4, 4);
        assert_eq!(board.rows, 4);
        assert_eq!(board.cols, 4);
        assert_eq!(board.get_cell(0, 0), None);
        board.set_cell(0, 0, Some(1));
        assert_eq!(board.get_cell(0, 0), Some(1));
        // Fill a row
        for c in 0..4 { board.set_cell(3, c, Some(2)); }
        assert_eq!(board.clear_full_lines(), 1);
        assert_eq!(board.grid[0], vec![None; 4]);
    }
}