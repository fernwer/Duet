The Analyst Profile:

Context:
I have a database with the TPC-H schema (tables: customer, orders, lineitem).
- customer (c_custkey, c_name, c_mktsegment)
- orders (o_orderkey, o_custkey, o_orderdate, o_totalprice)
- lineitem (l_orderkey, l_partkey, l_quantity, l_extendedprice)

Task:
I need to generate a batch of SQL queries to analyze order volumes for different market segments on different dates. 
Please generate 10 distinct SQL queries. 
Each query should calculate the total price of orders for a specific 'c_mktsegment' (e.g., 'BUILDING', 'AUTOMOBILE', 'MACHINERY') after a specific 'o_orderdate'.
Vary the market segment and the date randomly for each query.

Constraint:
- Return ONLY the raw SQL statements, one per line.
- Do not add explanations.

The Prober Profile:

Context:
I am an AI agent exploring a database. I know there is a table named 'orders' and 'lineitem', but I am unsure about the exact column names for "shipping date". It might be 'ship_date', 'l_shipdate', or 'shipping_date'.
Also, I want to check two different scenarios simultaneously to save time.

Task:
Generate 4 SQL queries to probe this database:
1. Query 1: Count orders where 'l_shipdate' is today (assuming the column is l_shipdate).
2. Query 2: Count orders where 'ship_date' is today (assuming the column is ship_date).
3. Query 3: Select all columns from 'orders' where 'o_orderdate' is '2023-01-01'.
4. Query 4: Select all columns from 'orders' where 'o_orderdate' is '2023-12-31'.

Constraint:
- Return ONLY the raw SQL statements.
- Allow potential schema errors (since you are guessing column names).

Syntactic Variance

Context:
Database Schema: TPC-H (customer, orders).
Goal: Find the names of customers who have placed an order with total price greater than 1000.

Task:
Write this specific SQL query in 5 completely different syntactic ways (e.g., using explicit JOIN, using implicit JOIN, using EXISTS, using IN, using CTE).

Constraint:
- Return ONLY the raw SQL statements.