#!/usr/bin/env python3
"""Compare SQLite SQ3 database contents without modifying either file."""

import argparse
import itertools
import sqlite3
from pathlib import Path
from urllib.parse import quote


def open_read_only(path: Path) -> sqlite3.Connection:
    uri = f"file:{quote(str(path.resolve()))}?mode=ro"
    connection = sqlite3.connect(uri, uri=True)
    # Token sequences may contain arbitrary bytes that are not valid UTF-8.
    connection.text_factory = bytes
    return connection


def table_names(connection: sqlite3.Connection, include_statistics: bool) -> set[str]:
    names = {
        row[0].decode("utf-8")
        for row in connection.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'"
        )
    }
    if not include_statistics:
        names.discard("Statistics")
    return names


def quote_identifier(identifier: str) -> str:
    return '"' + identifier.replace('"', '""') + '"'


def ordered_rows(connection: sqlite3.Connection, table: str):
    quoted_table = quote_identifier(table)
    columns = connection.execute(f"PRAGMA table_info({quoted_table})").fetchall()
    primary_key = [row[1].decode("utf-8") for row in sorted(columns, key=lambda row: row[5]) if row[5]]
    order_columns = primary_key or [row[1].decode("utf-8") for row in columns]
    order_by = ", ".join(quote_identifier(column) for column in order_columns)
    return connection.execute(f"SELECT * FROM {quoted_table} ORDER BY {order_by}")


def page_info(connection: sqlite3.Connection) -> tuple[int, int]:
    return (
        connection.execute("PRAGMA page_count").fetchone()[0],
        connection.execute("PRAGMA freelist_count").fetchone()[0],
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expected", type=Path, help="reference .sq3 file")
    parser.add_argument("actual", type=Path, help=".sq3 file to compare")
    parser.add_argument(
        "--include-statistics",
        action="store_true",
        help="compare the Statistics table too (timings and paths may differ)",
    )
    args = parser.parse_args()

    with open_read_only(args.expected) as expected, open_read_only(args.actual) as actual:
        expected_tables = table_names(expected, args.include_statistics)
        actual_tables = table_names(actual, args.include_statistics)
        same = expected_tables == actual_tables
        print(f"Expected file: {args.expected} ({args.expected.stat().st_size:,} bytes)")
        print(f"Actual file:   {args.actual} ({args.actual.stat().st_size:,} bytes)")
        for label, connection in (("expected", expected), ("actual", actual)):
            pages, free_pages = page_info(connection)
            print(f"{label} SQLite pages: {pages:,}; free pages: {free_pages:,}")

        if expected_tables != actual_tables:
            same = False
            print(f"Tables only in expected: {sorted(expected_tables - actual_tables)}")
            print(f"Tables only in actual:   {sorted(actual_tables - expected_tables)}")

        for table in sorted(expected_tables & actual_tables):
            expected_rows = ordered_rows(expected, table)
            actual_rows = ordered_rows(actual, table)
            row_count_expected = expected.execute(
                f"SELECT COUNT(*) FROM {quote_identifier(table)}"
            ).fetchone()[0]
            row_count_actual = actual.execute(
                f"SELECT COUNT(*) FROM {quote_identifier(table)}"
            ).fetchone()[0]
            first_difference = None
            for index, (expected_row, actual_row) in enumerate(
                itertools.zip_longest(expected_rows, actual_rows, fillvalue=None)
            ):
                if expected_row != actual_row:
                    first_difference = (index, expected_row, actual_row)
                    break
            table_matches = first_difference is None
            same &= table_matches
            print(
                f"{table}: {'MATCH' if table_matches else 'DIFFERENT'} "
                f"({row_count_expected:,} vs {row_count_actual:,} rows)"
            )
            if first_difference is not None:
                print(f"  first differing row: {first_difference}")

    print("Database contents match." if same else "Database contents differ.")
    return 0 if same else 1


if __name__ == "__main__":
    raise SystemExit(main())
