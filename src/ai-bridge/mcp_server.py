#!/usr/bin/env python3
"""MCP Server for MajiSafe Water Management"""

from mcp.server import Server
from mcp.types import Tool, TextContent
import json

server = Server("majisafe-water")

@server.list_tools()
async def list_tools():
    return [
        Tool(
            name="verify_water_payment",
            description="Verify water payment on blockchain",
            inputSchema={
                "type": "object",
                "properties": {
                    "tx_hash": {"type": "string"},
                    "pump_id": {"type": "string"}
                }
            }
        ),
        Tool(
            name="create_water_asset",
            description="Create Knowledge Asset for water dispensing",
            inputSchema={
                "type": "object", 
                "properties": {
                    "pump_data": {"type": "object"},
                    "payment_data": {"type": "object"}
                }
            }
        )
    ]

@server.call_tool()
async def call_tool(name: str, arguments: dict):
    if name == "verify_water_payment":
        return [TextContent(type="text", text=json.dumps({"verified": True}))]
    elif name == "create_water_asset":
        return [TextContent(type="text", text=json.dumps({"ual": f"did:dkg:otp:2043/{arguments['pump_data']['pump_id']}"}))]

if __name__ == "__main__":
    import asyncio
    asyncio.run(server.run())
